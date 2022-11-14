/************************************************************************************

Filename    :   Cinemastd::strings.h
Content     :	Text std::strings used by app.  Located in one place to make translation easier.
Created     :	9/30/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( CinemaStrings_h )
#define CinemaStrings_h

#include "OVR_Types.h"
#include <string>


namespace OculusCinema {

class CinemaApp;

class ovrCinemaStrings {
public:
	static ovrCinemaStrings *	Create( CinemaApp & cinema );
	static void					Destroy( CinemaApp & cinema, ovrCinemaStrings * & strings );

	void		OneTimeInit( CinemaApp &cinema );

	std::string     Category_LimeLight;
	std::string     Category_RemoteDesktop;


	std::string		MovieSelection_Resume;
	std::string		MovieSelection_Next;

	std::string		ResumeMenu_Title;
	std::string		ResumeMenu_Resume;
	std::string		ResumeMenu_Restart;

	std::string		TheaterSelection_Title;


	std::string      Error_NoVideosInLimeLight;

	std::string	ButtonText_ButtonSaveApp;
	std::string	ButtonText_ButtonSaveDefault;
	std::string	ButtonText_ButtonResetSettings;
	std::string	ButtonText_ButtonSaveSettings1;
	std::string	ButtonText_ButtonSaveSettings2;
	std::string	ButtonText_ButtonSaveSettings3;
	std::string	ButtonText_ButtonLoadSettings1;
	std::string	ButtonText_ButtonLoadSettings2;
	std::string	ButtonText_ButtonLoadSettings3;

	std::string    ButtonText_ButtonMapKeyboard;
	std::string    ButtonText_ButtonHostAudio;
	std::string    ButtonText_ButtonBitrate;
	std::string    ButtonText_ButtonApply;
	std::string    ButtonText_Button4k60;
	std::string    ButtonText_Button4k30;
	std::string    ButtonText_Button1080p60;
	std::string    ButtonText_Button1080p30;
	std::string    ButtonText_Button720p60;
	std::string    ButtonText_Button720p30;
	std::string    ButtonText_ButtonDistance;
	std::string    ButtonText_ButtonSize;
	std::string    ButtonText_ButtonSBS;
	std::string    ButtonText_ButtonChangeSeat;
	std::string    ButtonText_ButtonGaze;
	std::string    ButtonText_ButtonTrackpad;

	std::string    ButtonText_LabelTrackpadScale;


	std::string    ButtonText_ButtonOff;
	std::string    ButtonText_LabelGazeScale;
	std::string    ButtonText_LabelLatency;
	std::string    ButtonText_LabelVRXScale;
	std::string    ButtonText_LabelVRYScale;
	std::string    ButtonText_CloseApp;
	std::string    ButtonText_Settings;

	std::string    HelpText;

	std::string 	  Error_UnknownHost;
	std::string    Error_AddPCFailed;
	std::string    title_add_pc;


	std::string		Error_UnableToPlayMovie;

	std::string		MoviePlayer_Reorient;
};

} // namespace OculusCinema

#endif // Cinemastd::strings_h
