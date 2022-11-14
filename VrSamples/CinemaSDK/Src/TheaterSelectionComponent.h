/************************************************************************************

Filename    :   TheaterSelectionComponent.h
Content     :   Menu component for the movie theater selection menu.
Created     :   August 15, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GUI/Fader.h"
#include "CarouselBrowserComponent.h"

#if !defined( TheaterSelectionComponent_h )
#define TheaterSelectionComponent_h


namespace OculusCinema {

class TheaterSelectionView;

//==============================================================
// TheaterSelectionComponent
class TheaterSelectionComponent : public CarouselItemComponent
{
public:
	TheaterSelectionComponent( TheaterSelectionView *view );

	virtual void 			SetItem( OVRFW::VRMenuObject * self, const CarouselItem * item, const PanelPose &pose );

private:
    OVRFW::ovrSoundLimiter    		Sound;

    TheaterSelectionView * 	CallbackView;

    OVRFW::SineFader       		HilightFader;
    double          		StartFadeInTime;
    double          		StartFadeOutTime;
    float           		HilightScale;
    float           		FadeDuration;

private:
    virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                          OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

    OVRFW::eMsgStatus              FocusGained( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                         OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    OVRFW::eMsgStatus              FocusLost( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                       OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    OVRFW::eMsgStatus 				Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                     OVRFW::VRMenuObject * self,OVRFW:: VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // TheaterSelectionComponent_h
