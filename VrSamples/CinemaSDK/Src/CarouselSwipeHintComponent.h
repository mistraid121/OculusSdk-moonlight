/************************************************************************************

Filename    :   CarouselSwipeHintComponent.h
Content     :   Menu component for the swipe hints.
Created     :   October 6, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GUI/VRMenuComponent.h"
#include "GUI/Lerp.h"

#if !defined( CAROUSELSWIPEHINTCOMPONENT_H )
#define CAROUSELSWIPEHINTCOMPONENT_H


namespace OculusCinema {

class CarouselBrowserComponent;

//==============================================================
// CarouselSwipeHintComponent
class CarouselSwipeHintComponent : public OVRFW::VRMenuComponent
{
public:
	static const char *			TYPE_NAME;

	static bool					ShowSwipeHints;

	CarouselSwipeHintComponent( CarouselBrowserComponent *carousel, const bool isRightSwipe, const float totalTime, const float timeOffset, const float delay );

	virtual const char *		GetTypeName( ) const { return TYPE_NAME; }

	void						Reset( OVRFW::VRMenuObject * self );

private:
    CarouselBrowserComponent *	Carousel;
    bool 						IsRightSwipe;
    float 						TotalTime;
    float						TimeOffset;
    float 						Delay;
    double 						StartTime;
    bool						ShouldShow;
    bool						IgnoreDelay;
	OVRFW::Lerp						TotalAlpha;

private:
    bool 						CanSwipe() const;
    void 						Show( const double now );
    void 						Hide( const double now );
    virtual OVRFW::eMsgStatus      	OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												   OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              	Opening( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
											  OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              	Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
											OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // CAROUSELSWIPEHINTCOMPONENT_H
