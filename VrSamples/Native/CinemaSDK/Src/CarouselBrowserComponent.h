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

#include "VRMenu.h"
#include "VRMenuComponent.h"

#include <string>

using namespace OVR;

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
	Quatf    	Orientation;
	Vector3f 	Position;
	Vector4f	Color;

				PanelPose() {};
				PanelPose( Quatf orientation, Vector3f position, Vector4f color ) :
					Orientation( orientation ), Position( position ), Color( color ) {}
};

class CarouselItemComponent : public VRMenuComponent
{
public:
	explicit						CarouselItemComponent( VRMenuEventFlags_t const & eventFlags ) :
										VRMenuComponent( eventFlags )
									{
									}

	virtual							~CarouselItemComponent() { }

	virtual void 					SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose ) = 0;
};

class CarouselBrowserComponent : public VRMenuComponent
{
public:
									CarouselBrowserComponent( const std::vector<CarouselItem *> &items, const std::vector<PanelPose> &panelPoses );

	void							SetPanelPoses( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const std::vector<PanelPose> &panelPoses );
	void 							SetMenuObjects( const std::vector<VRMenuObject *> &menuObjs, const std::vector<CarouselItemComponent *> &menuComps );
	void							SetItems( const std::vector<CarouselItem *> &items );
	void							SetSelectionIndex( const int selectedIndex );
    int 							GetSelection() const;
	bool							HasSelection() const;
	bool							IsSwiping() const { return Swiping; }
	bool							CanSwipeBack() const;
	bool							CanSwipeForward() const;

	void 							CheckGamepad( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self );

private:
    virtual eMsgStatus 				OnEvent_Impl( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );
    PanelPose 						GetPosition( const float t );
    void 							UpdatePanels( OvrVRMenuMgr & menuMgr, VRMenuObject * self );

    eMsgStatus 						Frame( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus 						SwipeForward( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self );
    eMsgStatus 						SwipeBack( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self );
	eMsgStatus 						TouchDown( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						TouchUp( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						Opened( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						Closed( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame, VRMenuObject * self, VRMenuEvent const & event );

public:
    bool							SelectPressed;

private:
	Vector3f						PositionScale;
    float							Position;
	double							TouchDownTime;			// the time in second when a down even was received, < 0 if touch is not down

    std::vector<CarouselItem *> 			Items;
    std::vector<VRMenuObject *> 			MenuObjs;
    std::vector<CarouselItemComponent *> 	MenuComps;
	std::vector<PanelPose>					PanelPoses;

	double 							StartTime;
	double 							EndTime;
	float							PrevPosition;
	float							NextPosition;

	bool							Swiping;
	bool							PanelsNeedUpdate;
};

} // namespace OculusCinema

#endif // OVR_CarouselBrowser_h
