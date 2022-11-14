/************************************************************************************

Filename    :   ResumeMovieView.h
Content     :
Created     :	9/3/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( ResumeMovieView_h )
#define ResumeMovieView_h

#include "View.h"
#include "GUI/VRMenu.h"

namespace OculusCinema {

class CinemaApp;

class ResumeMovieView : public View
{
public:
						ResumeMovieView( CinemaApp &cinema );
	virtual 			~ResumeMovieView();

	virtual void 		OneTimeInit( const char * launchIntent );
	virtual void		OneTimeShutdown();

	virtual void 		OnOpen( const double currTimeInSeconds );
	virtual void 		OnClose();

	virtual bool 		OnKeyEvent( const int keyCode, const int repeatCount, const OVRFW::UIKeyboard::KeyEventType eventType );
	virtual void 		Frame( const OVRFW::ovrApplFrameIn & vrFrame );

	void 				ResumeChoice( int itemNum );

private:
	CinemaApp &			Cinema;

	static OVRFW::VRMenuId_t  	ID_CENTER_ROOT;
	static OVRFW::VRMenuId_t  	ID_TITLE;
	static OVRFW::VRMenuId_t  	ID_OPTIONS;
	static OVRFW::VRMenuId_t  	ID_OPTION_ICONS;

	OVRFW::VRMenu *			Menu;

private:
	ResumeMovieView &	operator=( const ResumeMovieView & );

	void				SetPosition( OVRFW::OvrVRMenuMgr & menuMgr, const OVR::Vector3f &pos );
	void 				CreateMenu( OVRFW::OvrGuiSys & guiSys );
};

} // namespace OculusCinema

#endif // ResumeMovieView_h
