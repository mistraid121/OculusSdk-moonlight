/************************************************************************************

Filename    :   MovieCategoryComponent.h
Content     :   Menu component for the movie category menu.
Created     :   August 13, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GUI/VRMenuComponent.h"
#include "PcManager.h"
#include "SelectionView.h"

#if !defined( MovieCategoryComponent_h )
#define MovieCategoryComponent_h


namespace OculusCinema {


//==============================================================
// MovieCategoryComponent
class PcCategoryComponent : public OVRFW::VRMenuComponent
{
public:
							PcCategoryComponent( SelectionView *view, PcCategory category );

	static const OVR::Vector4f	HighlightColor;
	static const OVR::Vector4f	FocusColor;
	static const OVR::Vector4f	NormalColor;

private:
    OVRFW::ovrSoundLimiter			Sound;

	bool					HasFocus;

    SelectionView *	CallbackView;

private:
    virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                    OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

	void					UpdateColor( OVRFW::VRMenuObject * self );

    OVRFW::eMsgStatus              Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                    OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    OVRFW::eMsgStatus              FocusGained( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                    OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
    OVRFW::eMsgStatus              FocusLost( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
                                    OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // MovieCategoryComponent_h
