/************************************************************************************

Filename    :   UIContainer.cpp
Content     :
Created     :	1/5/2015
Authors     :   Jim Dose

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "UI/UIContainer.h"
#include "UI/UIMenu.h"
#include "VRMenuMgr.h"
#include "GuiSys.h"
#include "App.h"

namespace OVR {

UIContainer::UIContainer( OvrGuiSys &guiSys ) :
	UIObject( guiSys )

{
}

UIContainer::~UIContainer()
{
}

void UIContainer::AddToMenu( UIMenu *menu, UIObject *parent )
{
	const Posef pose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	Vector3f defaultScale( 1.0f );
	VRMenuFontParms fontParms( HORIZONTAL_CENTER, VERTICAL_CENTER, false, false, false, 1.0f );
	
	VRMenuObjectParms parms( VRMENU_CONTAINER, std::vector< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"Container", pose, defaultScale, fontParms, menu->AllocId(),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	AddToMenuWithParms( menu, parent, parms );
}

} // namespace OVR
