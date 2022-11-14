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
	//SoundEffectPlayer( NULL ),
	VrFrame(),
	ViewMgr(),
	MoviePlayer( *this ),
	PcSelectionMenu( *this ),
	AppSelectionMenu( *this ),
	TheaterSelectionMenu( *this ),
	ResumeMovieMenu( *this ),
	MessageQueue( 100 ),
	FrameCount( 0 ),
	CurrentMovie( NULL ),
	PlayList(),
	ShouldResumeMovie( false ),
	MovieFinishedPlaying( false ),
	MountState( true ),			// We assume that the device is mounted at start since we can only detect changes in mount state
	LastMountState( true ),
	UseSrgb( false )
{
}

bool CinemaApp::AppInit( const OVRFW::ovrAppContext * context ) {
	GuiSys = OvrGuiSys::Create(context);
	if (nullptr == GuiSys) {
		ALOGE("Couldn't create GUI");
		return false;
	}

	StartTime = GetTimeInSeconds();

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
	ResumeMovieMenu.OneTimeShutdown();
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
/*
void CinemaApp::EnteredVrMode( const ovrIntentType intentType, const char * intentFromPackage, const char * intentJSON, const char * intentURI )
{
	OVR_UNUSED( intentFromPackage );
	OVR_UNUSED( intentJSON );

	if ( intentType == INTENT_LAUNCH )
	{
		OVR_LOG( "--------------- CinemaApp OneTimeInit ---------------");
//TODO: deplacer dans appinit?
		const ovrJava * java = app->GetJava();
		SoundEffectContext = new ovrSoundEffectContext( *java->Env, java->ActivityObject );
		SoundEffectContext->Initialize( &app->GetFileSys() );
		SoundEffectPlayer = new ovrGuiSoundEffectPlayer( *SoundEffectContext );

		Locale = ovrLocale::Create( *java->Env, java->ActivityObject, "default", &app->GetFileSys() );

		std::string fontName;
		GetLocale().GetString( "@string/font_name", "efigs.fnt", fontName );
		GuiSys->Init( this->app, *SoundEffectPlayer, fontName.c_str(), &app->GetDebugLines() );

		GuiSys->GetGazeCursor().ShowCursor();
	
		StartTime = GetTimeInSeconds();

		Native::OneTimeInit( ActivityClass );

		CinemaStrings = ovrCinemaStrings::Create( *this );

		ShaderMgr.OneTimeInit( intentURI );
		ModelMgr.OneTimeInit( intentURI );
		SceneMgr.OneTimeInit( intentURI );
	    	PcMgr.OneTimeInit( intentURI );
    		AppMgr.OneTimeInit( intentURI);
		MoviePlayer.OneTimeInit( intentURI );
		
		ViewMgr.AddView( &MoviePlayer );
		PcSelectionMenu.OneTimeInit( intentURI );
	    	ViewMgr.AddView( &PcSelectionMenu );
	    	AppSelectionMenu.OneTimeInit( intentURI );
	    	ViewMgr.AddView( &AppSelectionMenu );
	    	TheaterSelectionMenu.OneTimeInit( intentURI );

		ViewMgr.AddView( &TheaterSelectionMenu );
  		ResumeMovieMenu.OneTimeInit( intentURI );

		PcSelection( true );

		OVR_LOG( "CinemaApp::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - StartTime );
	}
	else if ( intentType == INTENT_NEW )
	{
	}

	OVR_LOG( "CinemaApp::EnteredVrMode" );
	// Clear cursor trails.
	GetGuiSys().GetGazeCursor().HideCursorForFrames( 10 );	
	ViewMgr.EnteredVrMode();
}
*/
void CinemaApp::LeavingVrMode()
{
	OVR_LOG( "CinemaApp::LeavingVrMode" );
	ViewMgr.LeavingVrMode();
}

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

void CinemaApp::SetPlaylist( const std::vector<const PcDef *> &playList, const int nextMovie )
{
	PlayList = playList;

	//OVR_ASSERT( nextMovie < PlayList.GetSizeI() );
	SetMovie( PlayList[ nextMovie ] );
}

void CinemaApp::SetMovie( const PcDef *movie )
{
	OVR_LOG( "SetMovie( %s )", movie->Name.c_str() );
	CurrentMovie = movie;
	MovieFinishedPlaying = false;
}

void CinemaApp::SetPc( const PcDef *pc )
{
    OVR_LOG( "SetPc( %s )", pc->Name.c_str() );
    CurrentPc = pc;
}

void CinemaApp::MovieLoaded( const int width, const int height, const int duration )
{
	MoviePlayer.MovieLoaded( width, height, duration );
}

void CinemaApp::StartMoviePlayback(int width, int height, int fps, bool hostAudio, int customBitrate)
{
	if ( CurrentMovie != NULL )
	{
		MovieFinishedPlaying = false;
		bool remote = CurrentPc->isRemote;
		Native::StartMovie(CurrentPc->UUID.c_str(), CurrentMovie->Name.c_str(), CurrentMovie->Id, CurrentPc->Binding.c_str(), width, height, fps, hostAudio, customBitrate, remote );

		ShouldResumeMovie = false;
	}
}

void CinemaApp::ResumeMovieFromSavedLocation()
{
	OVR_LOG( "ResumeMovie");
	InLobby = false;
	ShouldResumeMovie = true;
	ViewMgr.OpenView( MoviePlayer );
}

void CinemaApp::PlayMovieFromBeginning()
{
	OVR_LOG( "PlayMovieFromBeginning");
	InLobby = false;
	ShouldResumeMovie = false;
	ViewMgr.OpenView( MoviePlayer );
}

void CinemaApp::ResumeOrRestartMovie()
{
	
	PlayMovieFromBeginning();
	
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
    AppSelectionMenu.SetError(msg.c_str(),false,false);
}

void CinemaApp::PairSuccess()
{
    AppSelectionMenu.ClearError();
    AppSelectionMenu.PairSuccess();
}

void CinemaApp::ShowError( const std::string& msg )
{
    View *view = ViewMgr.GetCurrentView();
    if(view) view->SetError(msg.c_str(), false, true);
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

