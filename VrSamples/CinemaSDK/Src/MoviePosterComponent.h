/************************************************************************************

Filename    :   MoviePosterComponent.h
Content     :   Menu component for the movie selection menu.
Created     :   August 13, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( MoviePosterComponent_h )
#define MoviePosterComponent_h

#include "CarouselBrowserComponent.h"
#include "GUI/UI/UIImage.h"
#include "GUI/UI/UIContainer.h"

namespace OculusCinema {

class MovieDef;

//==============================================================
// MoviePosterComponent
class MoviePosterComponent : public CarouselItemComponent
{
public:
							MoviePosterComponent();

	static bool 			ShowShadows;

	void 					SetMenuObjects( const int width, const int height, OVRFW::UIContainer * poster, OVRFW::UIImage * posterImage, OVRFW::UIImage * is3DIcon, OVRFW::UIImage * shadow );
	virtual void 			SetItem( OVRFW::VRMenuObject * self, const CarouselItem * item, const PanelPose &pose );

private:
    virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												 OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

    const MovieDef *	 	Movie;

    int						Width;
    int						Height;

    OVRFW::UIContainer * 			Poster;
    OVRFW::UIImage * 				PosterImage;
    OVRFW::UIImage * 				Is3DIcon;
    OVRFW::UIImage * 				Shadow;
};

} // namespace OculusCinema

#endif // MoviePosterComponent_h
