/************************************************************************************

Filename    :   CinemaApp.cpp
Content     :   
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "CinemaApp.h"
#include "Native.h"
#include "CinemaStrings.h"
#include "Locale/OVR_Locale.h"
#include "Render/DebugLines.h"
#include "PackageFiles.h"

using namespace OVRFW;

//=======================================================================================

namespace OculusCinema {

//==============================================================
// ovrGuiSoundEffectPlayer
class ovrGuiSoundEffectPlayer : public OvrGuiSys::SoundEffectPlayer
{
public:
	ovrGuiSoundEffectPlayer( ovrSoundEffectContext & context )
		: SoundEffectContext( context )
	{
	}

	virtual bool Has( const char * name ) const OVR_OVERRIDE { return SoundEffectContext.GetMapping().HasSound( name ); }
	virtual void Play( const char * name ) OVR_OVERRIDE { SoundEffectContext.Play( name ); }

private:
	ovrGuiSoundEffectPlayer  &	operator=( const ovrGuiSoundEffectPlayer & );
	ovrSoundEffectContext & SoundEffectContext;
};

CinemaApp::CinemaApp(const int32_t mainThreadTid, const int32_t renderThreadTid,
					 const int cpuLevel, const int gpuLevel) : ovrAppl( mainThreadTid, renderThreadTid, cpuLevel, gpuLevel, true /* useMutliView */ ),
	Locale( NULL ),
	CinemaStrings( NULL ),
	StartTime( 0 ),
	SceneMgr( *this ),
	ShaderMgr( *this ),
	ModelMgr( *this ),
	PcMgr( *this ),
	AppMgr( *this ),
	InLobby( true ),
	AllowDebugControls( false ),
	SoundEffectContext( NULL ),
	SoundEffectPlayer( NULL ),
	VrFrame(),
	ViewMgr(),
	MoviePlayer( *this ),
	PcSelectionMenu( *this ),
	AppSelectionMenu( *this ),
	TheaterSelectionMenu( *this ),
	MessageQueue( 100 ),
	FrameCount( 0 ),
	CurrentApp( NULL ),
	PlayList(),
	MovieFinishedPlaying( false ),
	MountState( true ),			// We assume that the device is mounted at start since we can only detect changes in mount state
	LastMountState( true ),
	UseSrgb( false )
{
}

bool CinemaApp::AppInit( const OVRFW::ovrAppContext * appContext ) {
	ALOGV( "--------------- CinemaApp AppInit ---------------");
	StartTime = GetTimeInSeconds();

	/// Init File System / APK services
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appContext->ContextForVrApi() ) );
	JNIEnv *env;
	ctx.Vm->AttachCurrentThread(&env, 0);
	jobject me = ctx.ActivityObject;
	jclass acl = env->GetObjectClass(me); //class pointer of NativeActivity

	FileSys = OVRFW::ovrFileSys::Create( ctx );
	if ( nullptr == FileSys )
	{
		ALOGE( "AppInit - could not create FileSys" );
		return false;
	}

    /// Check to see if we can load resources from APK
    void * zipFile = ovr_GetApplicationPackageFile();
    if ( nullptr == zipFile )
    {
        char curPackageCodePath[OVRFW::ovrFileSys::OVR_MAX_PATH_LEN];
        ovr_GetPackageCodePath( ctx.Env, ctx.ActivityObject, curPackageCodePath, sizeof( curPackageCodePath ) );
        ovr_OpenApplicationPackage( curPackageCodePath, nullptr );
        ALOG( "curPackageCodePath = '%s' zipFile = %p", curPackageCodePath, zipFile );
        zipFile = nullptr;
    }

	GuiSys = OvrGuiSys::Create(appContext);
	if (nullptr == GuiSys) {
		ALOGE("Couldn't create GUI");
		return false;
	}

	Locale = ovrLocale::Create( *ctx.Env, ctx.ActivityObject, "default" );
	if ( nullptr == Locale )
	{
		ALOGE( "Couldn't create Locale" );
		return false;
	}

	SoundEffectContext = new ovrSoundEffectContext( *ctx.Env, ctx.ActivityObject );
	if ( nullptr == SoundEffectContext )
	{
		ALOGE( "Couldn't create SoundEffectContext" );
		return false;
	}
	SoundEffectContext->Initialize( FileSys );

	SoundEffectPlayer = new ovrGuiSoundEffectPlayer( *SoundEffectContext );
	if ( nullptr == SoundEffectPlayer )
	{
		ALOGE( "Couldn't create SoundEffectPlayer" );
		return false;
	}

	DebugLines = OvrDebugLines::Create();
	if ( nullptr == DebugLines )
	{
		ALOGE( "Couldn't create DebugLines" );
		return false;
	}
	DebugLines->Init();

	std::string fontName;//="efigs.fnt";
	GetLocale().GetLocalizedString( "@string/font_name", "efigs.fnt", fontName );
	GuiSys->Init( FileSys, *SoundEffectPlayer, fontName.c_str(), DebugLines );
	GuiSys->GetGazeCursor().ShowCursor();

	Native::OneTimeInit( acl );

	CinemaStrings = ovrCinemaStrings::Create( *this );

	/// Check launch intents for file override
	std::string intentURI;
	{
		jmethodID giid = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
		jobject intent = env->CallObjectMethod(me, giid); //Got our intent
		jclass icl = env->GetObjectClass(intent); //class pointer of Intent

		// Get Uri
		jmethodID igd = env->GetMethodID(icl, "getData", "()Landroid/net/Uri;");
		jobject uri = env->CallObjectMethod(intent, igd);
		if ( uri != NULL )
		{
			jclass ucl = env->FindClass("android/net/Uri"); //class pointer of Uri
			jmethodID uts = env->GetMethodID(ucl, "toString", "()Ljava/lang/String;");
			jstring uristr = (jstring) env->CallObjectMethod(uri, uts);
			if ( uristr )
			{
				const char * uristr_ch = env->GetStringUTFChars(uristr, 0);
				ALOGV( "AppInit - uristr_ch = `%s`", uristr_ch ) ;
				intentURI += uristr_ch;
				env->ReleaseStringUTFChars(uristr, uristr_ch);
			}
		}
	}
	ALOGV( "AppInit - intent = `%s`", intentURI.c_str() );

	ShaderMgr.OneTimeInit( intentURI.c_str() );
	ModelMgr.OneTimeInit( intentURI.c_str() );
	SceneMgr.OneTimeInit( intentURI.c_str() );
	PcMgr.OneTimeInit( intentURI.c_str() );
	AppMgr.OneTimeInit( intentURI.c_str());
	MoviePlayer.OneTimeInit( intentURI.c_str() );

	ViewMgr.AddView( &MoviePlayer );
	PcSelectionMenu.OneTimeInit( intentURI.c_str() );
	ViewMgr.AddView( &PcSelectionMenu );
	AppSelectionMenu.OneTimeInit( intentURI.c_str() );
	ViewMgr.AddView( &AppSelectionMenu );
	TheaterSelectionMenu.OneTimeInit( intentURI.c_str() );

	ViewMgr.AddView( &TheaterSelectionMenu );


	PcSelection( true );

	ALOGV( "CinemaApp::AppInit: %3.1f seconds", GetTimeInSeconds() - StartTime );

	// Clear cursor trails.
	GetGuiSys().GetGazeCursor().HideCursorForFrames( 10 );
	ViewMgr.EnteredVrMode();

	return true;
}
/*CinemaApp::~CinemaApp()
{
	OVR_LOG( "--------------- ~CinemaApp() ---------------");

	delete SoundEffectPlayer;
	SoundEffectPlayer = NULL;

	delete SoundEffectContext;
	SoundEffectContext = NULL;

	Native::OneTimeShutdown();
	ShaderMgr.OneTimeShutdown();
	ModelMgr.OneTimeShutdown();
	SceneMgr.OneTimeShutdown();
	PcMgr.OneTimeShutdown();
   	AppMgr.OneTimeShutdown();

	MoviePlayer.OneTimeShutdown();
	PcSelectionMenu.OneTimeShutdown();
    	AppSelectionMenu.OneTimeShutdown();
	TheaterSelectionMenu.OneTimeShutdown();
	ovrCinemaStrings::Destroy( *this, CinemaStrings );

	OvrGuiSys::Destroy( GuiSys );
}*/
/*
void CinemaApp::Configure( ovrSettings & settings )
{
	// We need very little CPU for movie playing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	settings.CpuLevel = 1;
	settings.GpuLevel = 2;

	settings.UseSrgbFramebuffer = UseSrgb;

	// Default to 2x MSAA.
	settings.EyeBufferParms.colorFormat = COLOR_8888;
	//settings.EyeBufferParms.depthFormat = DEPTH_16;
	settings.EyeBufferParms.multisamples = 2;

	settings.RenderMode = RENDERMODE_MULTIVIEW;
}
*/


const std::string CinemaApp::RetailDir( const char *dir ) const
{
	std::string subDir = "";
	subDir += SDCardDir( "RetailMedia" );
	subDir += "/";
	subDir += dir;
	return subDir;
}

const std::string CinemaApp::ExternalRetailDir( const char *dir ) const
{
	std::string subDir = "";
	subDir += ExternalSDCardDir( "RetailMedia" );
	subDir += "/";
	subDir += dir;
	return subDir;
}

const std::string CinemaApp::SDCardDir( const char *dir ) const
{
	std::string subDir = "";
	std::string sdcardPath;
	const ovrJava & java = *reinterpret_cast< const ovrJava* >( GetContext()->ContextForVrApi() );
	ovrFileSys::GetPathIfValidPermission( java,EST_PRIMARY_EXTERNAL_STORAGE, EFT_ROOT, "", permissionFlags_t( PERMISSION_READ ), sdcardPath );
	subDir += sdcardPath;
	subDir += dir;
	return subDir;
}

const std::string CinemaApp::ExternalSDCardDir( const char *dir ) const
{
	std::string subDir = "";
	std::string externalSdcardPath;
	const ovrJava & java = *reinterpret_cast< const ovrJava* >( GetContext()->ContextForVrApi() );
	ovrFileSys::GetPathIfValidPermission( java,EST_SECONDARY_EXTERNAL_STORAGE, EFT_ROOT, "", permissionFlags_t( PERMISSION_READ ), externalSdcardPath );
	subDir += externalSdcardPath;
	subDir += dir;
	return subDir;
}

bool CinemaApp::FileExists( const std::string & filename ) const
{
	FILE * f = fopen( filename.c_str(), "r" );
	if ( f == nullptr )
	{
		return false;
	}
	else
	{
		fclose( f );
		return true;
	}
}

bool CinemaApp::GetUseSrgb() const
{
	return UseSrgb;// && app->GetFramebufferIsSrgb();
}

void CinemaApp::SetPlaylist( const std::vector<const AppDef *> &playList, const int nextMovie )
{
	PlayList = playList;

	//OVR_ASSERT( nextMovie < PlayList.GetSizeI() );
	SetApp( PlayList[ nextMovie ] );
}

void CinemaApp::SetApp( const AppDef *app )
{
	OVR_LOG( "SetMovie( %s )", app->Name.c_str() );
	CurrentApp = app;
	MovieFinishedPlaying = false;
}

void CinemaApp::SetPc( const PcDef *pc )
{
    OVR_LOG( "SetPc( %s )", pc->Name.c_str() );
    CurrentPc = pc;
}

void CinemaApp::StartMoviePlayback(int width, int height, int fps, bool hostAudio, int customBitrate)
{
	if ( CurrentApp != NULL )
	{
		MovieFinishedPlaying = false;
		bool remote = CurrentPc->isRemote;
		Native::StartMovie(CurrentPc->UUID.c_str(), CurrentApp->Name.c_str(), CurrentApp->Id, CurrentPc->Binding.c_str(), width, height, fps, hostAudio, customBitrate, remote );
	}
}

void CinemaApp::PlayOrResumeOrRestartApp()
{
	OVR_LOG( "PlayOrResumeOrRestartApp");
	InLobby = false;
	ViewMgr.OpenView( MoviePlayer );
	
}

void CinemaApp::MovieFinished()
{
	InLobby = false;
	MovieFinishedPlaying = true;
	ViewMgr.OpenView( AppSelectionMenu );
}

void CinemaApp::UnableToPlayMovie()
{
	InLobby = false;
	//TODO rafa
	//AppSelectionMenu.SetError( CinemaStrings::Error_UnableToPlayMovie.c_str(), false, true );
   	ViewMgr.OpenView( AppSelectionMenu );

}

void CinemaApp::TheaterSelection()
{
	ViewMgr.OpenView( TheaterSelectionMenu );
}

void CinemaApp::PcSelection( bool inLobby )
{
	InLobby = inLobby;
	ViewMgr.OpenView( PcSelectionMenu );
}

void CinemaApp::AppSelection( bool inLobby )
{
	InLobby = inLobby;
	ViewMgr.OpenView( AppSelectionMenu );
}

bool CinemaApp::AllowTheaterSelection() const
{
    return true;
}

bool CinemaApp::IsMovieFinished() const
{
	return MovieFinishedPlaying;
}


const SceneDef & CinemaApp::GetCurrentTheater() const
{
	return ModelMgr.GetTheater( TheaterSelectionMenu.GetSelectedTheater() );
}

bool CinemaApp::OnKeyEvent( const int keyCode, const int repeatCount, const UIKeyboard::KeyEventType eventType )
{
	if ( GuiSys->OnKeyEvent( keyCode, repeatCount ) )
	{
		return true;
	}

	return ViewMgr.OnKeyEvent( keyCode, repeatCount,eventType );
}

void CinemaApp::ShowPair( const std::string& msg )
{
    AppSelectionMenu.SetError(msg.c_str(),false);
}

void CinemaApp::PairSuccess()
{
    AppSelectionMenu.ClearError();
    AppSelectionMenu.PairSuccess();
}

void CinemaApp::ShowError( const std::string& msg )
{
    View *view = ViewMgr.GetCurrentView();
    if(view) view->SetError(msg.c_str(), true);
}

void CinemaApp::ClearError()
{
    View *view = ViewMgr.GetCurrentView();
    if(view) view->ClearError();
}

void CinemaApp::Command( const char * msg )
{
	if ( SceneMgr.Command( msg ) )
	{
		return;
	}
}

ovrRendererOutput CinemaApp::Frame( const ovrApplFrameIn & vrFrame )
{
	//FrameResult = ovrFrameResult();

	// process input events first because this mirrors the behavior when OnKeyEvent was
	// a virtual function on VrAppInterface and was called by VrAppFramework.
	/*
	for ( int i = 0; i < vrFrame.Input.NumKeyEvents; i++ )
	{
		const int keyCode = vrFrame.Input.KeyEvents[i].KeyCode;
		const int repeatCount = vrFrame.Input.KeyEvents[i].RepeatCount;
		const UIKeyboard::KeyEventType eventType = vrFrame.Input.KeyEvents[i].EventType;

		if ( OnKeyEvent( keyCode, repeatCount, eventType ) )
		{
			continue;   // consumed the event
		}
		// If nothing consumed the key and it's a short-press of the back key, then exit the application to OculusHome.
		if ( keyCode == OVR_KEY_BACK && eventType == KEY_EVENT_SHORT_PRESS )
		{
			app->ShowConfirmQuitSystemUI();
			continue;
		}           
	}*/
	// Process incoming messages until the queue is empty.
	for ( ; ; )
	{
		const char * msg = MessageQueue.GetNextMessage();
		if ( msg == NULL )
		{
			break;
		}
		Command( msg );
		free( (void *)msg );
	}

	FrameCount++;
	VrFrame = vrFrame;

	// update mount state
	LastMountState = MountState;
	MountState = vrFrame.HeadsetMounted();
	if ( HeadsetWasMounted() )
	{
		OVR_LOG( "Headset mounted" );
	}
	else if ( HeadsetWasUnmounted() )
	{
		OVR_LOG( "Headset unmounted" );
	}

	// The View handles setting the FrameResult and Parms.
	ViewMgr.Frame( vrFrame );

	// Update gui systems after the app frame, but before rendering anything.
	GuiSys->Frame( vrFrame, FrameResult.FrameMatrices.CenterView );

	//-------------------------------
	// Rendering
	//-------------------------------

	GuiSys->AppendSurfaceList( FrameResult.FrameMatrices.CenterView, &FrameResult.Surfaces );

	return FrameResult;
}

ovrCinemaStrings & CinemaApp::GetCinemaStrings() const
{
	return *CinemaStrings;
}

void CinemaApp::MovieScreenUpdated()
{
	MoviePlayer.MovieScreenUpdated();
}



} // namespace OculusCinema

