/************************************************************************************

Filename    :   UIMenu.h
Content     :
Created     :	1/5/2015
Authors     :   Jim Dose

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#if !defined( UIMenu_h )
#define UIMenu_h

#include "VRMenu.h"
#include "GuiSys.h"

#include <string>

namespace OVR {

class UIMenu
{
	friend class UIMenuImpl;
public:
	using								OnFrameFunctionT =		void(*)( UIMenu * menu, void * usrPtr );
	using								OnKeyEventFunctionT =	bool(*)( UIMenu * menu, void * usrPtr, int const keyCode, const int repeatCount, KeyEventType const eventType );

										UIMenu( OvrGuiSys & guiSys );
										~UIMenu();

	VRMenuId_t 							AllocId();

	void 								Create( const char * menuName );
	void								Destroy();

	void 								Open();
	void 								Close();

	bool								IsOpen() const { return MenuOpen; }

	VRMenu *							GetVRMenu() const { return Menu; }

    VRMenuFlags_t const &				GetFlags() const;
	void								SetFlags( VRMenuFlags_t	const & flags );
	
	Posef const &						GetMenuPose() const;
	void								SetMenuPose( Posef const & pose );
	
	void								SetOnFrameFunction( OnFrameFunctionT onFrameFunction, void * usrPtr = nullptr );
	void								SetOnKeyEventFunction( OnKeyEventFunctionT onKeyEventFunction, void * usrPtr = nullptr );
private:
	OvrGuiSys &							GuiSys;
    std::string							MenuName;
	VRMenu *							Menu;

	bool								MenuOpen;

	VRMenuId_t							IdPool;

	OnFrameFunctionT					OnFrameFunction { nullptr };
	void *								OnFrameFunctionUsrPtr { nullptr };

	OnKeyEventFunctionT					OnKeyEventFunction { nullptr };
	void *								OnKeyEventFunctionUsrPtr { nullptr };

private:
	// private assignment operator to prevent copying
	UIMenu &	operator = ( UIMenu & );
};

} // namespace OVR

#endif // UIMenu_h
