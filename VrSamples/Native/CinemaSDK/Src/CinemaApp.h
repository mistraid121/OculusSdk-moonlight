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

#include "App.h"
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
#include "ResumeMovieView.h"
#include "GuiSys.h"
#include "SoundEffectContext.h"
#include <memory>
#include <string>

using namespace OVR;

namespace OVR {
	class ovrLocale;
}

namespace OculusCinema {

class ovrCinemaStrings;

class CinemaApp : public OVR::VrAppInterface
{
public:
							CinemaApp();
	//virtual					~CinemaApp();

	virtual void			Configure( ovrSettings & settings );
	virtual void			EnteredVrMode( const ovrIntentType intentType, const char * intentFromPackage, const char * intentJSON, const char * intentURI );
	virtual void			LeavingVrMode();
	virtual bool 			OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType );
	virtual ovrFrameResult	Frame( const ovrFrameInput & vrFrame );

	OvrGuiSys &				GetGuiSys() { return *GuiSys; }
	ovrLocale &				GetLocale() { return *Locale; }
	ovrMessageQueue &		GetMessageQueue() { return MessageQueue; }

	const ovrFrameInput &	GetFrame() const { return VrFrame; }
	ovrFrameResult &		GetFrameResult() { return FrameResult; }

	void                    SetPlaylist( const std::vector<const PcDef *> &playList, const int nextMovie );
    	void                    SetMovie( const PcDef * nextMovie );
    	void                    SetPc( const PcDef * pc);

	void 					MovieLoaded( const int width, const int height, const int duration );

	const PcDef *            GetCurrentMovie() const { return CurrentMovie; }
    	const PcDef *            GetCurrentPc() const { return CurrentPc; }
   	const PcDef *            GetNextMovie() const;
    	const PcDef *            GetPreviousMovie() const;


	const SceneDef & 		GetCurrentTheater() const;

	void                   			StartMoviePlayback(int width, int height, int fps, bool hostAudio, int customBitrate);
	void 					ResumeMovieFromSavedLocation();
	void					PlayMovieFromBeginning();
	void 					ResumeOrRestartMovie();
	void 					TheaterSelection();
	void                   			PcSelection( bool inLobby );
	void   			                AppSelection( bool inLobby );

	void					MovieFinished();
	void					UnableToPlayMovie();

	bool 					AllowTheaterSelection() const;
	bool 					IsMovieFinished() const;

	const std::string		RetailDir( const char *dir ) const;
	const std::string		ExternalRetailDir( const char *dir ) const;
	const std::string		SDCardDir( const char *dir ) const;
	const std::string 		ExternalSDCardDir( const char *dir ) const;
	const std::string 		ExternalCacheDir( const char *dir ) const;
	bool 					IsExternalSDCardDir( const char *dir ) const;
	bool 					FileExists( const std::string & filename ) const;

    	void                    		ShowPair( const std::string& msg );
	void                   			PairSuccess();
	void                    		ShowError( const std::string& msg );
	void                    		ClearError();
	void                    		MovieScreenUpdated();

	bool					HeadsetWasMounted() const { return ( MountState == true ) && ( LastMountState == false ); }
	bool					HeadsetWasUnmounted() const { return ( MountState == false ) && ( LastMountState == true ); }
	bool					HeadsetMountStateChanged() const { return ( MountState != LastMountState ); }
	bool					HeadsetMountState() const { return MountState; }

	bool					GetUseSrgb() const;

	ovrSoundEffectContext & GetSoundEffectContext() { return *SoundEffectContext; }
	ovrCinemaStrings &		GetCinemaStrings() const;

public:
	OvrGuiSys *				GuiSys;
	ovrLocale *				Locale;
	ovrCinemaStrings *		CinemaStrings;
	double					StartTime;

	jclass					MainActivityClass;	// need to look up from main thread

	SceneManager			SceneMgr;
	ShaderManager 			ShaderMgr;
	ModelManager 			ModelMgr;
	PcManager                 PcMgr;
	AppManager                AppMgr;

	bool					InLobby;
	bool					AllowDebugControls;

private:
	ovrSoundEffectContext * SoundEffectContext;
	OvrGuiSys::SoundEffectPlayer * SoundEffectPlayer;

	ovrFrameInput			VrFrame;
	ovrFrameResult			FrameResult;

	ViewManager				ViewMgr;
	MoviePlayerView			MoviePlayer;
	PcSelectionView         PcSelectionMenu;
   	AppSelectionView        AppSelectionMenu;

	TheaterSelectionView	TheaterSelectionMenu;
	ResumeMovieView			ResumeMovieMenu;

	ovrMessageQueue			MessageQueue;

	int						FrameCount;

    	const PcDef *            CurrentMovie;
    	const PcDef *            CurrentPc;
    	std::vector<const PcDef *>    PlayList;

	bool					ShouldResumeMovie;
	bool					MovieFinishedPlaying;

	bool					MountState;
	bool					LastMountState;

	bool					UseSrgb;

private:
	void 					Command( const char * msg );
};

} // namespace OculusCinema
