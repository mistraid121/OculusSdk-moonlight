/************************************************************************************

Filename    :   CinemaApp.h
Content     :   
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "Appl.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "SceneManager.h"
#include "ViewManager.h"
#include "PcManager.h"
#include "AppManager.h"
#include "MoviePlayerView.h"
#include "SelectionView.h"
#include "PcSelectionView.h"
#include "AppSelectionView.h"
#include "TheaterSelectionView.h"
#include "GUI/GuiSys.h"
#include "Sound/SoundEffectContext.h"
#include <memory>
#include <string>


namespace OVRFW {
	class ovrLocale;
}

namespace OculusCinema {

class ovrCinemaStrings;

class CinemaApp : public OVRFW::ovrAppl
{
public:
	CinemaApp(const int32_t mainThreadTid, const int32_t renderThreadTid,
			  const int cpuLevel, const int gpuLevel);

	virtual ~CinemaApp()
	{
	}

    // Called when the application initializes.
    // Must return true if the application initializes successfully.
    virtual bool            AppInit( const OVRFW::ovrAppContext * appContext ) override;

	//virtual void			Configure( ovrSettings & settings );
	virtual bool 			OnKeyEvent( const int keyCode, const int repeatCount, const OVRFW::UIKeyboard::KeyEventType eventType );
	//virtual OVRFW::ovrRendererOutput	Frame( const OVRFW::ovrApplFrameIn & vrFrame );
	// Called once per frame when the VR session is active.
	virtual OVRFW::ovrApplFrameOut	AppFrame( const OVRFW::ovrApplFrameIn & vrFrame ) override;
	// Called once per frame to allow the application to render eye buffers.
	virtual void                  	AppRenderFrame( const OVRFW::ovrApplFrameIn & in, OVRFW::ovrRendererOutput & out ) override;
	OVRFW::OvrGuiSys &				GetGuiSys() { return *GuiSys; }
	OVRFW::ovrLocale &				GetLocale() { return *Locale; }
    OVRFW::ovrMessageQueue &		GetMessageQueue() { return MessageQueue; }

	const OVRFW::ovrApplFrameIn &	GetFrame() const { return VrFrame; }

	void                    SetPlaylist( const std::vector<const AppDef *> &playList, const int nextApp );
	void                    SetApp( const AppDef * nextApp );
	void                    SetPc( const PcDef * pc);

	void 					MovieLoaded( const int width, const int height, const int duration );

	const AppDef *            GetCurrentApp() const { return CurrentApp; }
	const PcDef *            GetCurrentPc() const { return CurrentPc; }
	const SceneDef & 		GetCurrentTheater() const;

	void                   	StartMoviePlayback(int width, int height, int fps, bool hostAudio, int customBitrate);
	void 					PlayOrResumeOrRestartApp();
	void 					TheaterSelection();
	void                   	PcSelection( bool inLobby );
	void   			        AppSelection( bool inLobby );

	void					MovieFinished();
	void					UnableToPlayMovie();

	bool 					AllowTheaterSelection() const;
	bool 					IsMovieFinished() const;

	const std::string		RetailDir( const char *dir ) const;
	const std::string		ExternalRetailDir( const char *dir ) const;
	const std::string		SDCardDir( const char *dir ) const;
	const std::string 		ExternalSDCardDir( const char *dir ) const;
	bool 					FileExists( const std::string & filename ) const;

    	void                    		ShowPair( const std::string& msg );
	void                   			PairSuccess();
	void                    		ShowError( const std::string& msg );
	void                    		ClearError();
	void                    		MovieScreenUpdated();

	bool					HeadsetWasMounted() const { return ( MountState == true ) && ( LastMountState == false ); }
	bool					HeadsetWasUnmounted() const { return ( MountState == false ) && ( LastMountState == true ); }
	bool					GetUseSrgb() const;

	OVRFW::ovrSoundEffectContext & GetSoundEffectContext() { return *SoundEffectContext; }
	ovrCinemaStrings &		GetCinemaStrings() const;

	float 					GetSuggestedEyeFovDegreesX() const {return SuggestedEyeFovDegreesX;}
	float 					GetSuggestedEyeFovDegreesY() const {return SuggestedEyeFovDegreesY;}

    int				  GetNumFramebuffers() const
    {
        return ovrAppl::GetNumFramebuffers();
    }

    ovrFramebuffer * GetFrameBuffer( int eye )
    {
        return ovrAppl::GetFrameBuffer(eye);
    }

    void 			AppEyeGLStateSetup( const OVRFW::ovrApplFrameIn & in, const ovrFramebuffer * fb, int eye ) override
    {
        ovrAppl::AppEyeGLStateSetup( in,  fb, eye );
    }

public:
	OVRFW::OvrGuiSys *				GuiSys;
	OVRFW::ovrLocale *				Locale;
	ovrCinemaStrings *		CinemaStrings;
	double					StartTime;

	SceneManager			SceneMgr;
	ShaderManager 			ShaderMgr;
	ModelManager 			ModelMgr;
	PcManager                 PcMgr;
	AppManager                AppMgr;

	bool					InLobby;
	bool					AllowDebugControls;

private:
	OVRFW::ovrSoundEffectContext * SoundEffectContext;
	OVRFW::OvrGuiSys::SoundEffectPlayer * SoundEffectPlayer;

	OVRFW::ovrApplFrameIn			VrFrame;
	OVRFW::ovrRendererOutput			FrameResult;

	OVRFW::ovrFileSys *				FileSys;
	OVRFW::OvrDebugLines *				DebugLines;

	ViewManager				ViewMgr;
	MoviePlayerView			MoviePlayer;
	PcSelectionView         PcSelectionMenu;
   	AppSelectionView        AppSelectionMenu;

	TheaterSelectionView	TheaterSelectionMenu;

    OVRFW::ovrMessageQueue			MessageQueue;

	const AppDef *            CurrentApp;
	const PcDef *            CurrentPc;
	std::vector<const AppDef *>    PlayList;

	bool					MovieFinishedPlaying;

	bool					MountState;
	bool					LastMountState;

	bool					UseSrgb;

private:
	void 					Command( const char * msg );
};

} // namespace OculusCinema
