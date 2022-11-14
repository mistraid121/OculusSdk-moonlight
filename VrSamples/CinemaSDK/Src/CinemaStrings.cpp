/************************************************************************************

Filename    :   CinemaStrings.cpp
Content     :	Text strings used by app.  Located in one place to make translation easier.
Created     :	9/30/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "CinemaStrings.h"
#include "Locale/OVR_Locale.h"
#include "CinemaApp.h"

using namespace OVRFW;

namespace OculusCinema
{

void ovrCinemaStrings::OneTimeInit( CinemaApp & cinema )
{
	OVR_LOG( "ovrCinemaStrings::OneTimeInit" );

	cinema.GetLocale().GetLocalizedString( "@string/Category_LimeLight", 	"@string/Category_LimeLight", 		Category_LimeLight );
	cinema.GetLocale().GetLocalizedString( "@string/Category_RemoteDesktop", 		"@string/Category_RemoteDesktop", 		Category_RemoteDesktop );

	cinema.GetLocale().GetLocalizedString( "@string/MovieSelection_Resume",	"@string/MovieSelection_Resume",	MovieSelection_Resume );
	cinema.GetLocale().GetLocalizedString( "@string/MovieSelection_Next", 	"@string/MovieSelection_Next", 		MovieSelection_Next );
	cinema.GetLocale().GetLocalizedString( "@string/ResumeMenu_Title", 		"@string/ResumeMenu_Title", 		ResumeMenu_Title );
	cinema.GetLocale().GetLocalizedString( "@string/ResumeMenu_Resume", 		"@string/ResumeMenu_Resume", 		ResumeMenu_Resume );
	cinema.GetLocale().GetLocalizedString( "@string/ResumeMenu_Restart", 	"@string/ResumeMenu_Restart", 		ResumeMenu_Restart );
	cinema.GetLocale().GetLocalizedString( "@string/TheaterSelection_Title", "@string/TheaterSelection_Title", 	TheaterSelection_Title );


	cinema.GetLocale().GetLocalizedString( "@string/Error_NoVideosInLimeLight", "@string/Error_NoVideosInLimeLight", Error_NoVideosInLimeLight );

	//cinema.GetLocale().GetLocalizedString( "@string/Error_UnableToPlayMovie", "@string/Error_UnableToPlayMovie",	Error_UnableToPlayMovie );

	cinema.GetLocale().GetLocalizedString( "@string/MoviePlayer_Reorient", 	"@string/MoviePlayer_Reorient", 	MoviePlayer_Reorient );

	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonMapKeyboard", 	"@string/ButtonText_ButtonMapKeyboard", 	ButtonText_ButtonMapKeyboard );

	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonHostAudio", 	"@string/ButtonText_ButtonHostAudio", 	ButtonText_ButtonHostAudio );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonBitrate", 	"@string/ButtonText_ButtonBitrate", 	ButtonText_ButtonBitrate);
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonApply", 	"@string/ButtonText_ButtonApply", 	ButtonText_ButtonApply);
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button4k30", 	"@string/ButtonText_Button4k30", 	ButtonText_Button4k30 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button4k60", 	"@string/ButtonText_Button4k60", 	ButtonText_Button4k60 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button1080p30", 	"@string/ButtonText_Button1080p30", 	ButtonText_Button1080p30 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button1080p60", 	"@string/ButtonText_Button1080p60", 	ButtonText_Button1080p60 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button720p30", 	"@string/ButtonText_Button720p30", 	    ButtonText_Button720p30 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Button720p60", 	"@string/ButtonText_Button720p60", 	    ButtonText_Button720p60 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonDistance", 	"@string/ButtonText_ButtonDistance", 	ButtonText_ButtonDistance );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSize", 	"@string/ButtonText_ButtonSize", 	ButtonText_ButtonSize);
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSBS", 	"@string/ButtonText_ButtonSBS", 	ButtonText_ButtonSBS );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonChangeSeat", 	"@string/ButtonText_ButtonChangeSeat", 	ButtonText_ButtonChangeSeat );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonGaze", 	"@string/ButtonText_ButtonGaze", 	ButtonText_ButtonGaze );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonTrackpad", 	"@string/ButtonText_ButtonTrackpad", 	ButtonText_ButtonTrackpad );

	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonOff", 	"@string/ButtonText_ButtonOff", 	ButtonText_ButtonOff );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_LabelGazeScale", 	"@string/ButtonText_LabelGazeScale", 	ButtonText_LabelGazeScale );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_LabelTrackpadScale", 	"@string/ButtonText_LabelTrackpadScale", 	ButtonText_LabelTrackpadScale );

	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_LabelLatency", 	"@string/ButtonText_LabelLatency",  	ButtonText_LabelLatency );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_LabelVRXScale", 	"@string/ButtonText_LabelVRXScale", 	ButtonText_LabelVRXScale );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_LabelVRYScale", 	"@string/ButtonText_LabelVRYScale", 	ButtonText_LabelVRYScale );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_CloseApp", 	    "@string/ButtonText_CloseApp", 	        ButtonText_CloseApp );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_Settings", 	    "@string/ButtonText_Settings",       	ButtonText_Settings );
	cinema.GetLocale().GetLocalizedString( "@string/HelpText", 	                "@string/HelpText", 	                HelpText );

	cinema.GetLocale().GetLocalizedString( "@string/Error_UnknownHost", 	"@string/Error_UnknownHost", 	Error_UnknownHost );
	cinema.GetLocale().GetLocalizedString( "@string/title_add_pc", 	"@string/title_add_pc", 	title_add_pc );

	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSaveApp", 	"@string/ButtonText_ButtonSaveApp", 	ButtonText_ButtonSaveApp );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSaveDefault", 	"@string/ButtonText_ButtonSaveDefault", 	ButtonText_ButtonSaveDefault );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonResetSettings", 	"@string/ButtonText_ButtonResetSettings", 	ButtonText_ButtonResetSettings );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSaveSettings1", 	"@string/ButtonText_ButtonSaveSettings1", 	ButtonText_ButtonSaveSettings1 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSaveSettings2", 	"@string/ButtonText_ButtonSaveSettings2", 	ButtonText_ButtonSaveSettings2 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonSaveSettings3", 	"@string/ButtonText_ButtonSaveSettings3", 	ButtonText_ButtonSaveSettings3 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonLoadSettings1", 	"@string/ButtonText_ButtonLoadSettings1", 	ButtonText_ButtonLoadSettings1 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonLoadSettings2", 	"@string/ButtonText_ButtonLoadSettings2", 	ButtonText_ButtonLoadSettings2 );
	cinema.GetLocale().GetLocalizedString( "@string/ButtonText_ButtonLoadSettings3", 	"@string/ButtonText_ButtonLoadSettings3", 	ButtonText_ButtonLoadSettings3);
}

ovrCinemaStrings *	ovrCinemaStrings::Create( CinemaApp & cinema )
{
	ovrCinemaStrings * cs = new ovrCinemaStrings;
	cs->OneTimeInit( cinema );
	return cs;
}

void ovrCinemaStrings::Destroy( CinemaApp & cinema, ovrCinemaStrings * & strings )
{
	OVR_UNUSED( cinema );

	delete strings;
	strings = NULL;
}

} // namespace OculusCinema
