/************************************************************************************

Filename    :   ResumeMovieComponent.h
Content     :
Created     :   September 4, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.
*************************************************************************************/

#include "GUI/VRMenuComponent.h"

#if !defined( ResumeMovieComponent_h )
#define ResumeMovieComponent_h

namespace OculusCinema {

class ResumeMovieView;

//==============================================================
// ResumeMovieComponent
class ResumeMovieComponent : public OVRFW::VRMenuComponent
{
public:
	ResumeMovieComponent( ResumeMovieView *view, int itemNum );

	OVRFW::VRMenuObject * 			Icon;

	static const OVR::Vector4f	HighlightColor;
	static const OVR::Vector4f	FocusColor;
	static const OVR::Vector4f	NormalColor;

private:
	OVRFW::ovrSoundLimiter			Sound;

	bool					HasFocus;
    //int						ItemNum;
    ResumeMovieView *		CallbackView;

private:
    virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												 OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

	void					UpdateColor( OVRFW::VRMenuObject * self );

	OVRFW::eMsgStatus              Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
										  OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              FocusGained( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              FocusLost( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
											  OVRFW::VRMenuObject * self,OVRFW:: VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // ResumeMovieComponent_h
