/************************************************************************************

Filename    :   TheaterSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GUI/GazeCursor.h"
#include "GUI/VRMenuMgr.h"
#include "GUI/GuiSys.h"
#include "CarouselBrowserComponent.h"
#include "TheaterSelectionComponent.h"
#include "TheaterSelectionView.h"
#include "CinemaApp.h"
#include "CarouselSwipeHintComponent.h"
#include "PackageFiles.h"
#include "CinemaStrings.h"
#include "Native.h"

using namespace OVRFW;
using OVR::Vector4f;
using OVR::Vector3f;
using OVR::Posef;
using OVR::Quatf;


namespace OculusCinema {

VRMenuId_t TheaterSelectionView::ID_CENTER_ROOT( 1000 );
VRMenuId_t TheaterSelectionView::ID_ICONS( 1001 );
VRMenuId_t TheaterSelectionView::ID_TITLE_ROOT( 2000 );
VRMenuId_t TheaterSelectionView::ID_SWIPE_ICON_LEFT( 3000 );
VRMenuId_t TheaterSelectionView::ID_SWIPE_ICON_RIGHT( 3001 );

static const int NumSwipeTrails = 3;

TheaterSelectionView::TheaterSelectionView( CinemaApp &cinema ) :
	View( "TheaterSelectionView" ),
	Cinema( cinema ),
	Menu( NULL ),
	CenterRoot( NULL ),
	SelectionObject( NULL ),
	TheaterBrowser( NULL ),
	SelectedTheater( 0 ),
	IgnoreSelectTime( 0 )

{
// This is called at library load time, so the system is not initialized
// properly yet.
}

TheaterSelectionView::~TheaterSelectionView()
{
}

void TheaterSelectionView::OneTimeInit( const char * launchIntent )
{
	OVR_UNUSED( launchIntent );

	OVR_LOG( "TheaterSelectionView::OneTimeInit" );

	const double start = GetTimeInSeconds();

	// Start with "Home theater" selected
	SelectedTheater = 0;

	OVR_LOG( "TheaterSelectionView::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - start );
}

void TheaterSelectionView::OneTimeShutdown()
{
	OVR_LOG( "TheaterSelectionView::OneTimeShutdown" );
}

void TheaterSelectionView::SelectTheater( int theater )
{
	SelectedTheater = theater;

	Cinema.SceneMgr.SetSceneModel( Cinema.ModelMgr.GetTheater( SelectedTheater ) );
	SetPosition( Cinema.GetGuiSys().GetVRMenuMgr(), Cinema.SceneMgr.Scene.GetFootPos() );
}

void TheaterSelectionView::OnOpen( const double currTimeInSeconds )
{
	OVR_LOG( "OnOpen" );

	if ( Menu == NULL )
	{
		CreateMenu( Cinema.GetGuiSys() );
	}

	SelectTheater( SelectedTheater );
	TheaterBrowser->SetSelectionIndex( SelectedTheater );

	Cinema.SceneMgr.LightsOn( 0.5f, currTimeInSeconds );

	Cinema.SceneMgr.ClearGazeCursorGhosts();
	Cinema.GetGuiSys().OpenMenu( "TheaterSelectionBrowser" );

	// ignore clicks for 0.5 seconds to avoid accidentally clicking through
	IgnoreSelectTime = currTimeInSeconds + 0.5;

	CurViewState = VIEWSTATE_OPEN;
}

void TheaterSelectionView::OnClose()
{
	OVR_LOG( "OnClose" );

	Cinema.GetGuiSys().CloseMenu( Menu, false );

	CurViewState = VIEWSTATE_CLOSED;
}

bool TheaterSelectionView::OnKeyEvent( const int keyCode, const int repeatCount, const UIKeyboard::KeyEventType eventType )
{
	OVR_UNUSED( keyCode );
	OVR_UNUSED( repeatCount );
	OVR_UNUSED( eventType );
	switch ( keyCode ) {/*
		case OVR_KEY_BACK: {
			switch (eventType) {
				case KEY_EVENT_SHORT_PRESS:
					OVR_LOG("KEY_EVENT_SHORT_PRESS");
					BackPressed();
					return true;
					break;
				default:
					//OVR_LOG( "unexpected back key state %i", eventType );
					break;
			}
		}*/
	}
	return false;
}

void TheaterSelectionView::SetPosition( OvrVRMenuMgr & menuMgr, const Vector3f & pos )
{
	Posef pose = CenterRoot->GetLocalPose();
	pose.Translation = pos;
	CenterRoot->SetLocalPose( pose );

	menuHandle_t titleRootHandle = Menu->HandleForId( menuMgr, ID_TITLE_ROOT );
	VRMenuObject * titleRoot = menuMgr.ToObject( titleRootHandle );
	OVR_ASSERT( titleRoot != NULL );

	pose = titleRoot->GetLocalPose();
	pose.Translation = pos;
	titleRoot->SetLocalPose( pose );
}

void TheaterSelectionView::CreateMenu( OvrGuiSys & guiSys )
{
	Menu = VRMenu::Create( "TheaterSelectionBrowser" );

	Vector3f fwd( 0.0f, 0.0f, 1.0f );
	Vector3f up( 0.0f, 1.0f, 0.0f );
	Vector3f defaultScale( 1.0f );

	std::vector< VRMenuObjectParms const * > parms;

	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );

	Quatf orientation( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f );
	Vector3f centerPos( 0.0f, 0.0f, 0.0f );

	VRMenuObjectParms centerRootParms( VRMENU_CONTAINER, std::vector< VRMenuComponent* >(), VRMenuSurfaceParms(), "CenterRoot",
			Posef( orientation, centerPos ), Vector3f( 1.0f, 1.0f, 1.0f ), fontParms,
			ID_CENTER_ROOT, VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.push_back( &centerRootParms );

	// title
	const Posef titlePose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms titleRootParms( VRMENU_CONTAINER, std::vector< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"TitleRoot", titlePose, defaultScale, fontParms, ID_TITLE_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.push_back( &titleRootParms );

	Menu->InitWithItems( guiSys, 0.0f, VRMenuFlags_t(), parms );
	parms.clear();

	int count = Cinema.ModelMgr.GetTheaterCount();
	for ( int i = 0 ; i < count ; i++ )
	{
		const SceneDef & theater = Cinema.ModelMgr.GetTheater( i );

		CarouselItem *item = new CarouselItem();
		item->Texture = theater.IconTexture;

		Theaters.push_back( item );
	}

	std::vector<PanelPose> panelPoses;

	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f( -2.85f, 1.8f, -5.8f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f( -1.90f, 1.8f, -5.0f ), Vector4f( 0.25f, 0.25f, 0.25f, 1.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f( -0.95f, 1.8f, -4.2f ), Vector4f( 0.45f, 0.45f, 0.45f, 1.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f(  0.00f, 1.8f, -3.4f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f(  0.95f, 1.8f, -4.2f ), Vector4f( 0.45f, 0.45f, 0.45f, 1.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f(  1.90f, 1.8f, -5.0f ), Vector4f( 0.25f, 0.25f, 0.25f, 1.0f ) ) );
	panelPoses.push_back( PanelPose( Quatf( up, 0.0f ), Vector3f(  2.85f, 1.8f, -5.8f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );

	// the centerroot item will get touch relative and touch absolute events and use them to rotate the centerRoot
	menuHandle_t centerRootHandle = Menu->HandleForId( guiSys.GetVRMenuMgr(), ID_CENTER_ROOT );
	CenterRoot = guiSys.GetVRMenuMgr().ToObject( centerRootHandle );
	OVR_ASSERT( CenterRoot != NULL );

	TheaterBrowser = new CarouselBrowserComponent( Theaters, panelPoses );
	CenterRoot->AddComponent( TheaterBrowser );

	int selectionWidth = 0;
	int selectionHeight = 0;
	GLuint selectionTexture = LoadTextureFromApplicationPackage( "assets/VoidTheater.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), selectionWidth, selectionHeight );

	int centerIndex = static_cast< int >( panelPoses.size() ) / 2;
	for ( int i = 0; i < static_cast< int >( panelPoses.size() ); ++i )
	{
		VRMenuSurfaceParms panelSurfParms( "",
				selectionTexture, selectionWidth, selectionHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

	    VRMenuId_t panelId = VRMenuId_t( ID_ICONS.Get() + i );
		Quatf rot( up, 0.0f );
		Posef panelPose( rot, fwd );
		VRMenuObjectParms * p = new VRMenuObjectParms( VRMENU_BUTTON, std::vector< VRMenuComponent* >(),
				panelSurfParms, "", panelPose, defaultScale, fontParms, panelId,
				( i == centerIndex ) ? VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) : VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMENUOBJECT_FLAG_NO_DEPTH,
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
		parms.push_back( p );
	}

	Menu->AddItems( guiSys, parms, centerRootHandle, false );
	DeletePointerArray( parms );
	parms.clear();

	menuHandle_t selectionHandle = CenterRoot->ChildHandleForId( guiSys.GetVRMenuMgr(), VRMenuId_t( ID_ICONS.Get() + centerIndex ) );
	SelectionObject = guiSys.GetVRMenuMgr().ToObject( selectionHandle );

	Vector3f selectionBoundsExpandMin = Vector3f( 0.0f, -0.25f, 0.0f );
	Vector3f selectionBoundsExpandMax = Vector3f( 0.0f, 0.25f, 0.0f );
	SelectionObject->SetLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );
	SelectionObject->AddFlags( VRMENUOBJECT_HIT_ONLY_BOUNDS );

	std::vector<VRMenuObject *> menuObjs;
	std::vector<CarouselItemComponent *> menuComps;
	for ( int i = 0; i < static_cast< int >( panelPoses.size() ); ++i )
	{
		menuHandle_t posterImageHandle = CenterRoot->ChildHandleForId( guiSys.GetVRMenuMgr(), VRMenuId_t( ID_ICONS.Get() + i ) );
		VRMenuObject *posterImage = guiSys.GetVRMenuMgr().ToObject( posterImageHandle );
		menuObjs.push_back( posterImage );

		CarouselItemComponent *panelComp = new TheaterSelectionComponent( i == centerIndex ? this : NULL );
		posterImage->AddComponent( panelComp );
		menuComps.push_back( panelComp );
	}
	TheaterBrowser->SetMenuObjects( menuObjs, menuComps );

	// ==============================================================================
	//
	// title
	//
	{
		Posef panelPose( Quatf( up, 0.0f ), Vector3f( 0.0f, 2.5f, -3.4f ) );

		VRMenuFontParms titleFontParms( true, true, false, false, false, 1.3f );

		VRMenuObjectParms p( VRMENU_STATIC, std::vector< VRMenuComponent* >(),
				VRMenuSurfaceParms(), Cinema.GetCinemaStrings().TheaterSelection_Title.c_str(), panelPose, defaultScale, titleFontParms, VRMenuId_t( ID_TITLE_ROOT.Get() + 1 ),
				VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.push_back( &p );

		menuHandle_t titleRootHandle = Menu->HandleForId( guiSys.GetVRMenuMgr(), ID_TITLE_ROOT );
		Menu->AddItems( guiSys, parms, titleRootHandle, false );
		parms.clear();
	}

	// ==============================================================================
	//
	// swipe icons
	//
	{
		int 	swipeIconLeftWidth = 0;
		int		swipeIconLeftHeight = 0;
		int 	swipeIconRightWidth = 0;
		int		swipeIconRightHeight = 0;

		GLuint swipeIconLeftTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowLeft.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), swipeIconLeftWidth, swipeIconLeftHeight );

		GLuint swipeIconRightTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowRight.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), swipeIconRightWidth, swipeIconRightHeight );

		VRMenuSurfaceParms swipeIconLeftSurfParms( "",
				swipeIconLeftTexture, swipeIconLeftWidth, swipeIconLeftHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		VRMenuSurfaceParms swipeIconRightSurfParms( "",
				swipeIconRightTexture, swipeIconRightWidth, swipeIconRightHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		float yPos = 1.8f - ( selectionHeight - swipeIconLeftHeight ) * 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		for ( int i = 0; i < NumSwipeTrails; i++ )
		{
			Posef swipePose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, yPos, -3.4f ) );
			swipePose.Translation.x = ( ( selectionWidth + swipeIconLeftWidth * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE;
			swipePose.Translation.z += 0.01f * ( float )i;

			std::vector< VRMenuComponent* > leftComps;
			leftComps.push_back( new CarouselSwipeHintComponent( TheaterBrowser, false, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconLeftParms = new VRMenuObjectParms( VRMENU_BUTTON, leftComps,
				swipeIconLeftSurfParms, "", swipePose, defaultScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_LEFT.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

			parms.push_back( swipeIconLeftParms );

			swipePose.Translation.x = ( ( selectionWidth + swipeIconRightWidth * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE;

			std::vector< VRMenuComponent* > rightComps;
			rightComps.push_back( new CarouselSwipeHintComponent( TheaterBrowser, true, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconRightParms = new VRMenuObjectParms( VRMENU_STATIC, rightComps,
				swipeIconRightSurfParms, "", swipePose, defaultScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_RIGHT.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

			parms.push_back( swipeIconRightParms );
		}

		Menu->AddItems( guiSys, parms, centerRootHandle, false );
		DeletePointerArray( parms );
		parms.clear();
	}

	Cinema.GetGuiSys().AddMenu( Menu );
}

void TheaterSelectionView::SelectPressed( const double currTimeInSeconds )
{
	const double now = currTimeInSeconds;
	if ( now < IgnoreSelectTime )
	{
		// ignore selection for first 0.5 seconds to reduce chances of accidentally clicking through
		return;
	}

	Cinema.SceneMgr.SetSceneModel( Cinema.ModelMgr.GetTheater( SelectedTheater ) );
	if ( Cinema.InLobby )
	{
		Cinema.PlayOrResumeOrRestartApp();
	}
	else
	{
		Cinema.PlayOrResumeOrRestartApp();
	}
}

void TheaterSelectionView::Frame( const ovrApplFrameIn & vrFrame )
{/*
	// We want 4x MSAA in the selection screen
	ovrEyeBufferParms eyeBufferParms = Cinema.app->GetEyeBufferParms();
	eyeBufferParms.multisamples = 4;
	Cinema.app->SetEyeBufferParms( eyeBufferParms );
*/
	if ( SelectionObject->IsHilighted() )
	{
		TheaterBrowser->CheckGamepad( Cinema.GetGuiSys(), vrFrame, CenterRoot );
		float touchpadMinSwipe = 100.0f;
		TheaterBrowser->CheckTouchpad( Cinema.GetGuiSys(), vrFrame,touchpadMinSwipe);
	}

	int selectedItem = TheaterBrowser->GetSelection();
	if ( SelectedTheater != selectedItem )
	{
		if ( ( selectedItem >= 0 ) && ( selectedItem < static_cast< int >( Theaters.size() ) ) )
		{
			OVR_LOG( "Select: %d, %d, %d, %d", selectedItem, SelectedTheater, static_cast< int >( Theaters.size() ), Cinema.ModelMgr.GetTheaterCount() );
			SelectedTheater = selectedItem;
			SelectTheater( SelectedTheater );
		}
	}

	if ( Menu->IsClosedOrClosing() && !Menu->IsOpenOrOpening() )
	{

		Cinema.AppSelection( false );
	}
/*
	if (vrFrame.AllButtons & BUTTON_B )
	{
		Cinema.GetSoundEffectContext().Play( "touch_up" );


        Cinema.AppSelection( false );
	}
*/
	//Cinema.SceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
