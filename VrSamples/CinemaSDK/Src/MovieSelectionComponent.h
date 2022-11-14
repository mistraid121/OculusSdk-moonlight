/************************************************************************************

Filename    :   MovieSelectionComponent.h
Content     :   Menu component for the movie selection menu.
Created     :   October 8, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GUI/VRMenuComponent.h"
#include "SelectionView.h"


#if !defined( MovieSelectionComponent_h )
#define MovieSelectionComponent_h

namespace OculusCinema {

class MovieSelectionView;

//==============================================================
// MovieSelectionComponent
class MovieSelectionComponent : public OVRFW::VRMenuComponent
{
public:
							MovieSelectionComponent( SelectionView *view );

private:
	OVRFW::ovrSoundLimiter			Sound;
    SelectionView *	CallbackView;

private:
    virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												 OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

	OVRFW::eMsgStatus              Frame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
										  OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              FocusGained( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
	OVRFW::eMsgStatus              FocusLost( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
											  OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // MovieSelectionComponent_h
