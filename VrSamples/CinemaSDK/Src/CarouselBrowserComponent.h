/************************************************************************************

Filename    :   CarouselBrowserComponent.h
Content     :   A menu for browsing a hierarchy of folders with items represented by thumbnails.
Created     :   July 25, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( OVR_CarouselBrowser_h )
#define OVR_CarouselBrowser_h

#include "GUI/VRMenu.h"
#include "GUI/VRMenuComponent.h"

#include <string>


namespace OculusCinema {

class CarouselItem
{
public:
	std::string	Name;
	GLuint		Texture;
	int			TextureWidth;
	int			TextureHeight;
	void *		UserData;

	CarouselItem() : Texture( 0 ), TextureWidth( 0 ), TextureHeight( 0 ), UserData( NULL ) {}
};

class PanelPose
{
public:
	OVR::Quatf    	Orientation;
	OVR::Vector3f 	Position;
	OVR::Vector4f	Color;

	PanelPose() {};
	PanelPose( OVR::Quatf orientation, OVR::Vector3f position, OVR::Vector4f color ) :
	Orientation( orientation ), Position( position ), Color( color ) {}
};

class CarouselItemComponent : public OVRFW::VRMenuComponent
{
public:
	explicit						CarouselItemComponent( OVRFW::VRMenuEventFlags_t const & eventFlags ) :
										OVRFW::VRMenuComponent( eventFlags )
									{
									}

	virtual							~CarouselItemComponent() { }

	virtual void 					SetItem( OVRFW::VRMenuObject * self, const CarouselItem * item, const PanelPose &pose ) = 0;
};

class CarouselBrowserComponent : public OVRFW::VRMenuComponent
{
public:
									CarouselBrowserComponent( const std::vector<CarouselItem *> &items, const std::vector<PanelPose> &panelPoses );

	void 							SetMenuObjects( const std::vector<OVRFW::VRMenuObject *> &menuObjs, const std::vector<CarouselItemComponent *> &menuComps );
	void							SetItems( const std::vector<CarouselItem *> &items );
	void							SetSelectionIndex( const int selectedIndex );
    int 							GetSelection() const;
	bool							HasSelection() const;
	bool							IsSwiping() const { return Swiping; }
	bool							CanSwipeBack() const;
	bool							CanSwipeForward() const;

	void 							CheckGamepad( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuObject * self );
	void 							CheckTouchpad(OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, const float min_swipe_distance );


private:
    virtual OVRFW::eMsgStatus 				OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    PanelPose 						GetPosition( const float t );
    void 							UpdatePanels( OVRFW::OvrVRMenuMgr & menuMgr, OVRFW::VRMenuObject * self );

    OVRFW::eMsgStatus 						Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    OVRFW::eMsgStatus 						SwipeForward( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame );
    OVRFW::eMsgStatus 						SwipeBack( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame );
	OVRFW::eMsgStatus 						Opened( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus 						Closed( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );


private:
	OVR::Vector3f						PositionScale;
    float							Position;
	
    std::vector<CarouselItem *> 			Items;
    std::vector<OVRFW::VRMenuObject *> 			MenuObjs;
    std::vector<CarouselItemComponent *> 	MenuComps;
	std::vector<PanelPose>					PanelPoses;

	double 							StartTime;
	double 							EndTime;
	float							PrevPosition;
	float							NextPosition;

	bool							Swiping;
	bool							PanelsNeedUpdate;

	double							lastTouchpadTime,touchpadTimer;
	bool							lastTouchDown;
	OVR::Vector2f					touchOrigin,touchRelative;
	int								touchState;
};

} // namespace OculusCinema

#endif // OVR_CarouselBrowser_h
