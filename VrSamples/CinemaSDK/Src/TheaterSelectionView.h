/************************************************************************************

Filename    :   TheaterSelectionView.h
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( TheaterSelectionView_h )
#define TheaterSelectionView_h

#include "View.h"
#include "GUI/GuiSys.h"

#include <vector>

namespace OculusCinema {

class CinemaApp;
class CarouselBrowser;
class CarouselItem;

class TheaterSelectionView : public View
{
public:
								TheaterSelectionView( CinemaApp &cinema );
	virtual 					~TheaterSelectionView();

	virtual void 				OneTimeInit( const char * launchIntent );
	virtual void				OneTimeShutdown();

	virtual void 				OnOpen( const double currTimeInSeconds );
	virtual void 				OnClose();
	virtual bool 				OnKeyEvent( const int keyCode, const int repeatCount, const OVRFW::UIKeyboard::KeyEventType eventType );
	virtual void 				Frame( const OVRFW::ovrApplFrameIn & vrFrame );

	void 						SelectPressed( const double currTimeInSeconds );
	void 						SelectTheater( int theater );

	int							GetSelectedTheater() const { return SelectedTheater; }

private:
	CinemaApp &					Cinema;

	static OVRFW::VRMenuId_t  			ID_CENTER_ROOT;
	static OVRFW::VRMenuId_t			ID_ICONS;
	static OVRFW::VRMenuId_t  			ID_TITLE_ROOT;
	static OVRFW::VRMenuId_t 			ID_SWIPE_ICON_LEFT;
	static OVRFW::VRMenuId_t 			ID_SWIPE_ICON_RIGHT;

	OVRFW::VRMenu *					Menu;
	OVRFW::VRMenuObject * 				CenterRoot;
	OVRFW::VRMenuObject * 				SelectionObject;

	CarouselBrowserComponent *	TheaterBrowser;
	std::vector<CarouselItem *> Theaters;

	int							SelectedTheater;

	double						IgnoreSelectTime;

private:
	TheaterSelectionView &		operator=( const TheaterSelectionView & );

	void						SetPosition( OVRFW::OvrVRMenuMgr & menuMgr, const OVR::Vector3f &pos );
	void 						CreateMenu( OVRFW::OvrGuiSys & guiSys );
};

} // namespace OculusCinema

#endif // TheaterSelectionView_h
