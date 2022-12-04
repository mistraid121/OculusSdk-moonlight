/************************************************************************************

Filename    :   PcSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <android/keycodes.h>
#include "PcSelectionView.h"
#include "CinemaApp.h"
#include "GUI/VRMenuMgr.h"
#include "GUI/GuiSys.h"
#include "PcCategoryComponent.h"
#include "MoviePosterComponent.h"
#include "MovieSelectionComponent.h"
#include "CarouselSwipeHintComponent.h"
#include "PackageFiles.h"
#include "CinemaStrings.h"
#include "Native.h"

using namespace OVRFW;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Vector4f;
using OVR::Matrix4f;
using OVR::Quatf;
using OVR::Bounds3f;

namespace OculusCinema {

static const int PosterWidth = 228;
static const int PosterHeight = 344;

static const Vector3f PosterScale( 4.4859375f * 0.98f );


static const int NumSwipeTrails = 3;


//=======================================================================================

PcSelectionView::PcSelectionView( CinemaApp &cinema ) :
	SelectionView( "PcSelectionView" ),
	Cinema( cinema ),
	SelectionTexture(),
	ShadowTexture(),
	BorderTexture(),
	SwipeIconLeftTexture(),
	SwipeIconRightTexture(),
	ResumeIconTexture(),
	ErrorIconTexture(),
	CloseIconTexture(),
	ErrorMessageClicked( false ),
	CenterIndex( 0 ),
	CenterPosition(),
	LeftSwipes(),
	RightSwipes(),
	MoveScreenAlpha(),
	SelectionFader(),
	PcPanelPositions(),
	PcPosterComponents(),
	Categories(),
	CurrentCategory( CATEGORY_LIMELIGHT ),
	PcList(),
	PcsIndex( 0 ),
	RepositionScreen( false ),
	HadSelection( false ),
	newPCWidth( 0 ),
	newPCHeight( 0 ),
	newPCTex(),
	bgTintTexture(),
	newPCIPButtons(),
	IPoctets(),
	currentOctet(0),
	IPString("0_.0.0.0")


{
	// This is called at library load time, so the system is not initialized
	// properly yet.
}

PcSelectionView::~PcSelectionView()
{
	DeletePointerArray( PcBrowserItems );
}

void PcSelectionView::OneTimeInit( const char * launchIntent )
{
	OVR_UNUSED( launchIntent );

	OVR_LOG( "PcSelectionView::OneTimeInit" );

	const double start = GetTimeInSeconds();

	CreateMenu( Cinema.GetGuiSys() );

	SetCategory( CATEGORY_LIMELIGHT );

    Native::InitPcSelector();

	OVR_LOG( "PcSelectionView::OneTimeInit %3.1f seconds",  GetTimeInSeconds() - start );
}

void PcSelectionView::OneTimeShutdown()
{
	OVR_LOG( "PcSelectionView::OneTimeShutdown" );
}

void PcSelectionView::OnOpen(const double currTimeInSeconds )
{
	OVR_LOG( "OnOpen" );
	CurViewState = VIEWSTATE_OPEN;

	LastPcDisplayed = NULL;
	HadSelection = false;

	if ( Cinema.InLobby )
	{
		Cinema.SceneMgr.SetSceneModel( *Cinema.ModelMgr.BoxOffice );

		Vector3f size( PosterWidth * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x, PosterHeight * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y, 0.0f );

		Cinema.SceneMgr.SceneScreenBounds = Bounds3f( size * -0.5f, size * 0.5f );
		Cinema.SceneMgr.SceneScreenBounds.Translate( Vector3f(  0.00f, 1.76f,  -7.25f ) );
	}

	Cinema.SceneMgr.LightsOn( 1.5f ,currTimeInSeconds);

	const double now = vrapi_GetTimeInSeconds();
	SelectionFader.Set( now, 0, now + 0.1, 1.0f );

	CategoryRoot->SetVisible( true );
	Menu->SetFlags( VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP );


	ResumeIcon->SetVisible( false );
	CenterRoot->SetVisible( true );
	
	MoveScreenLabel->SetVisible( false );

	PcBrowser->SetSelectionIndex( PcsIndex );

	RepositionScreen = false;
	MoveScreenAlpha.Set( 0, 0, 0, 0.0f );

	UpdateMenuPosition();
	Cinema.SceneMgr.ClearGazeCursorGhosts();
	Menu->Open();

	MoviePosterComponent::ShowShadows = Cinema.InLobby;

	CarouselSwipeHintComponent::ShowSwipeHints = true;
}

void PcSelectionView::OnClose()
{
	OVR_LOG( "OnClose" );
	CurViewState = VIEWSTATE_CLOSED;
	CenterRoot->SetVisible( false );
	Menu->Close();
	Cinema.SceneMgr.ClearMovie();
}

bool PcSelectionView::BackPressed()
{
	/*if(ErrorShown())
	{
		ClearError();
		return true;
	}
	if(newPCMenu->GetVisible())
	{
		newPCMenu->SetVisible(false);
		IPoctets[0] = IPoctets[1] = IPoctets[2] = IPoctets[3] = 0;
		currentOctet = 0;
		IPString = "_.0.0.0";

		return true;
	}
	 */
	return false;
}

//=======================================================================================

void NewPCIPButtonCallback( UIButton *button, void *object )
{
	( ( PcSelectionView * )object )->NewPCIPButtonPressed(button);
}


void CloseAppButtonCallback( UIButton *button, void *object )
{
	( ( PcSelectionView * )object )->CloseAppButtonPressed();
}

void PcSelectionView::CreateMenu( OvrGuiSys & guiSys )
{
	OVR_UNUSED( guiSys );

	const Quatf forward( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f );

	// ==============================================================================
	//
	// load textures
	//
	SelectionTexture.LoadTextureFromApplicationPackage( "assets/selection.png" );
	ShadowTexture.LoadTextureFromApplicationPackage( "assets/shadow.png" );
	BorderTexture.LoadTextureFromApplicationPackage( "assets/category_border.png" );
	SwipeIconLeftTexture.LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowLeft.png" );
	SwipeIconRightTexture.LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowRight.png" );
	ResumeIconTexture.LoadTextureFromApplicationPackage( "assets/resume.png" );
	ErrorIconTexture.LoadTextureFromApplicationPackage( "assets/error.png" );
	CloseIconTexture.LoadTextureFromApplicationPackage( "assets/close.png" );

	newPCTex = LoadTextureFromApplicationPackage(
			"assets/generic_add_poster.png",
			TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), newPCWidth, newPCHeight);
	bgTintTexture.LoadTextureFromApplicationPackage( "assets/backgroundTint.png" );

    ButtonTexture.LoadTextureFromApplicationPackage( "assets/button_num.png" );
	ButtonHoverTexture.LoadTextureFromApplicationPackage( "assets/button_num_hoover.png" );
	ButtonPressedTexture.LoadTextureFromApplicationPackage( "assets/button_num_pressed.png" );



	// ==============================================================================
	//
	// create menu
	//
	Menu = new UIMenu( Cinema.GetGuiSys() );
	Menu->Create( "PcBrowser" );

	CenterRoot = new UIContainer( Cinema.GetGuiSys() );
	CenterRoot->AddToMenu( Menu );
	CenterRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	PcRoot = new UIContainer( Cinema.GetGuiSys() );
	PcRoot->AddToMenu( Menu, CenterRoot );
	PcRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	CategoryRoot = new UIContainer( Cinema.GetGuiSys() );
	CategoryRoot->AddToMenu( Menu, CenterRoot );
	CategoryRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	TitleRoot = new UIContainer( Cinema.GetGuiSys() );
	TitleRoot->AddToMenu( Menu, CenterRoot );
	TitleRoot->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );

	// ==============================================================================
	//
	// error message
	//
	ErrorMessage = new UILabel( Cinema.GetGuiSys() );
	ErrorMessage->AddToMenu( Menu, CenterRoot );
	ErrorMessage->SetLocalPose( forward, Vector3f( 0.00f, 1.76f, -7.39f + 0.5f ) );
	ErrorMessage->SetLocalScale( Vector3f( 5.0f ) );
	ErrorMessage->SetFontScale( 0.5f );
	ErrorMessage->SetTextOffset( Vector3f( 0.0f, -48 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );
	ErrorMessage->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ErrorIconTexture );
	ErrorMessage->SetVisible( false );

	// ==============================================================================
	//
	// error without icon
	//
	PlainErrorMessage = new UILabel( Cinema.GetGuiSys() );
	PlainErrorMessage->AddToMenu( Menu, CenterRoot );
	PlainErrorMessage->SetLocalPose( forward, Vector3f( 0.00f, 1.76f + ( 330.0f - 48 ) * VRMenuObject::DEFAULT_TEXEL_SCALE, -7.39f + 0.5f ) );
	PlainErrorMessage->SetLocalScale( Vector3f( 5.0f ) );
	PlainErrorMessage->SetFontScale( 0.5f );
	PlainErrorMessage->SetVisible( false );

	// ==============================================================================
	//
	// Pc browser
	//
	PcPanelPositions.push_back( PanelPose( forward, Vector3f( -5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f( -3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f( -2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f(  0.00f, 1.76f,  -7.39f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f(  2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f(  3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	PcPanelPositions.push_back( PanelPose( forward, Vector3f(  5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );

	CenterIndex = PcPanelPositions.size() / 2;
	CenterPosition = PcPanelPositions[ CenterIndex ].Position;

	PcBrowser = new CarouselBrowserComponent( PcBrowserItems, PcPanelPositions );
	PcRoot->AddComponent( PcBrowser );

	// ==============================================================================
	//
	// selection rectangle
	//
	SelectionFrame = new UIImage( Cinema.GetGuiSys() );
	SelectionFrame->AddToMenu( Menu, PcRoot );
	SelectionFrame->SetLocalPose( forward, CenterPosition + Vector3f( 0.0f, 0.0f, 0.1f ) );
	SelectionFrame->SetLocalScale( PosterScale );
	SelectionFrame->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture );
	SelectionFrame->AddComponent( new MovieSelectionComponent( this ) );

	const Vector3f selectionBoundsExpandMin = Vector3f( 0.0f );
	const Vector3f selectionBoundsExpandMax = Vector3f( 0.0f, -0.13f, 0.0f );
	SelectionFrame->GetMenuObject()->SetLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );

	// ==============================================================================
	//
	// add shadow to Pc poster panels
	//
	std::vector<VRMenuObject *> menuObjs;
	for ( OVR::UPInt i = 0; i < PcPanelPositions.size(); ++i )
	{
		UIContainer *posterContainer = new UIContainer( Cinema.GetGuiSys() );
		posterContainer->AddToMenu( Menu, PcRoot );
		posterContainer->SetLocalPose( PcPanelPositions[ i ].Orientation, PcPanelPositions[ i ].Position );
		posterContainer->GetMenuObject()->AddFlags( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED );

		//
		// posters
		//
		UIImage * posterImage = new UIImage( Cinema.GetGuiSys() );
		posterImage->AddToMenu( Menu, posterContainer );
		posterImage->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, 0.0f ) );
		posterImage->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SelectionTexture.Texture, PosterWidth, PosterHeight );
		posterImage->SetLocalScale( PosterScale );
		posterImage->GetMenuObject()->AddFlags( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED );
		posterImage->GetMenuObject()->SetLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );

		if ( i == CenterIndex )
		{
			CenterPoster = posterImage;
		}

		//
		// shadow
		//
		UIImage * shadow = new UIImage( Cinema.GetGuiSys() );
		shadow->AddToMenu( Menu, posterContainer );
		shadow->SetLocalPose( forward, Vector3f( 0.0f, -1.97f, 0.00f ) );
		shadow->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ShadowTexture );
		shadow->SetLocalScale( PosterScale );
		shadow->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

		//
		// add the component
		//
		MoviePosterComponent *posterComp = new MoviePosterComponent();
		posterComp->SetMenuObjects( PosterWidth, PosterHeight, posterContainer, posterImage, shadow );
		posterContainer->AddComponent( posterComp );

		menuObjs.push_back( posterContainer->GetMenuObject() );
		PcPosterComponents.push_back( posterComp );
	}

	PcBrowser->SetMenuObjects( menuObjs, PcPosterComponents );

	// ==============================================================================
	//
	// category browser
	//
	Categories.push_back( PcCategoryButton( CATEGORY_LIMELIGHT, Cinema.GetCinemaStrings().Category_LimeLight ) );

	// create the buttons and calculate their size
	const float itemWidth = 1.10f;
	float categoryBarWidth = 0.0f;
	for ( OVR::UPInt i = 0; i < Categories.size(); ++i )
	{
		Categories[ i ].Button = new UILabel( Cinema.GetGuiSys() );
		Categories[ i ].Button->AddToMenu( Menu, CategoryRoot );
		Categories[ i ].Button->SetFontScale( 2.2f );
		Categories[ i ].Button->SetText( Categories[ i ].Text.c_str() );
		Categories[ i ].Button->AddComponent( new PcCategoryComponent( this, Categories[ i ].Category ) );

		const Bounds3f & bounds = Categories[ i ].Button->GetTextLocalBounds( Cinema.GetGuiSys().GetDefaultFont() );
		Categories[ i ].Width = std::max( bounds.GetSize().x, itemWidth ) + 80.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		Categories[ i ].Height = bounds.GetSize().y + 108.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		categoryBarWidth += Categories[ i ].Width;
	}

	// reposition the buttons and set the background and border
 	float startX = categoryBarWidth * -0.5f;
	for ( OVR::UPInt i = 0; i < Categories.size(); ++i )
	{
		VRMenuSurfaceParms panelSurfParms( "",
				BorderTexture.Texture, BorderTexture.Width, BorderTexture.Height, SURFACE_TEXTURE_ADDITIVE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		panelSurfParms.Border = Vector4f( 14.0f );
		panelSurfParms.Dims = Vector2f( Categories[ i ].Width * VRMenuObject::TEXELS_PER_METER, Categories[ i ].Height * VRMenuObject::TEXELS_PER_METER );

		Categories[ i ].Button->SetImage( 0, panelSurfParms );
		Categories[ i ].Button->SetLocalPose( forward, Vector3f( startX + Categories[ i ].Width * 0.5f, 3.6f, -7.39f ) );
		Categories[ i ].Button->SetLocalBoundsExpand( Vector3f( 0.0f, 0.13f, 0.0f ), Vector3f::ZERO );

    	startX += Categories[ i ].Width;
	}

	// ==============================================================================
	//
	// Pc title
	//
	PcTitle = new UILabel( Cinema.GetGuiSys() );
	PcTitle->AddToMenu( Menu, TitleRoot );
	PcTitle->SetLocalPose( forward, Vector3f( 0.0f, 0.045f, -7.37f ) );
	PcTitle->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	PcTitle->SetFontScale( 2.5f );

	// ==============================================================================
	//
	// swipe icons
	//
	float yPos = 1.76f - ( PosterHeight - SwipeIconLeftTexture.Height ) * 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y;

	for ( int i = 0; i < NumSwipeTrails; i++ )
	{
		Vector3f swipeIconPos = Vector3f( 0.0f, yPos, -7.17f + 0.01f * ( float )i );

		LeftSwipes[ i ] = new UIImage( Cinema.GetGuiSys() );
		LeftSwipes[ i ]->AddToMenu( Menu, CenterRoot );
		LeftSwipes[ i ]->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SwipeIconLeftTexture );
		LeftSwipes[ i ]->SetLocalScale( PosterScale );
		LeftSwipes[ i ]->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
		LeftSwipes[ i ]->AddComponent( new CarouselSwipeHintComponent( PcBrowser, false, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

		swipeIconPos.x = ( ( PosterWidth + SwipeIconLeftTexture.Width * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		LeftSwipes[ i ]->SetLocalPosition( swipeIconPos );

		RightSwipes[ i ] = new UIImage( Cinema.GetGuiSys() );
		RightSwipes[ i ]->AddToMenu( Menu, CenterRoot );
		RightSwipes[ i ]->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SwipeIconRightTexture );
		RightSwipes[ i ]->SetLocalScale( PosterScale );
		RightSwipes[ i ]->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
		RightSwipes[ i ]->AddComponent( new CarouselSwipeHintComponent( PcBrowser, true, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

		swipeIconPos.x = ( ( PosterWidth + SwipeIconRightTexture.Width * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		RightSwipes[ i ]->SetLocalPosition( swipeIconPos );
    }

	// ==============================================================================
	//
	// resume icon
	//
	ResumeIcon = new UILabel( Cinema.GetGuiSys() );
	ResumeIcon->AddToMenu( Menu, PcRoot );
	ResumeIcon->SetLocalPose( forward, CenterPosition + Vector3f( 0.0f, 0.0f, 0.5f ) );
	ResumeIcon->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, ResumeIconTexture );
	ResumeIcon->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	ResumeIcon->SetFontScale( 0.3f );
	ResumeIcon->SetLocalScale( 6.0f );
	ResumeIcon->SetText( Cinema.GetCinemaStrings().MovieSelection_Resume.c_str() );
	ResumeIcon->SetTextOffset( Vector3f( 0.0f, -ResumeIconTexture.Height * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f, 0.0f ) );
	ResumeIcon->SetVisible( false );

	// ==============================================================================
	//
	// close app button
	//
	CloseAppButton = new UIButton( Cinema.GetGuiSys()  );
	CloseAppButton->AddToMenu(  Menu, PcRoot );
	CloseAppButton->SetLocalPose( forward, CenterPosition + Vector3f( 0.8f, -1.28f, 0.5f ) );
	CloseAppButton->SetButtonImages( CloseIconTexture, CloseIconTexture, CloseIconTexture );
	CloseAppButton->SetLocalScale( 3.0f );
	CloseAppButton->SetVisible( false );
	CloseAppButton->SetOnClick( CloseAppButtonCallback, this );


	// ==============================================================================
	//
	// reorient message
	//
	MoveScreenLabel = new UILabel( Cinema.GetGuiSys() );
	MoveScreenLabel->AddToMenu( Menu, NULL );
	MoveScreenLabel->SetLocalPose( forward, Vector3f( 0.0f, 0.0f, -1.8f ) );
	MoveScreenLabel->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	MoveScreenLabel->SetFontScale( 0.5f );
	MoveScreenLabel->SetText( Cinema.GetCinemaStrings().MoviePlayer_Reorient );
	MoveScreenLabel->SetTextOffset( Vector3f( 0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );  // offset to be below gaze cursor
	MoveScreenLabel->SetVisible( false );

	// ==============================================================================
	//
	// IP Entry
	//
	newPCMenu= new UIContainer( Cinema.GetGuiSys() );
	newPCMenu->AddToMenu(  Menu );
	newPCMenu->SetLocalPose( forward, Vector3f( 0.0f, 1.5f, -3.0f ) );
	newPCMenu->SetVisible(false);

	newPCbg=new UIImage( Cinema.GetGuiSys() );
	newPCbg->AddToMenu(  Menu, newPCMenu);
    newPCbg->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 550, 1000 );


	newPCIPLabel=new UILabel( Cinema.GetGuiSys() );
	newPCIPLabel->AddToMenu(  Menu, newPCbg );
	newPCIPLabel->SetLocalPosition( Vector3f( 0.0f, 0.8f, 0.1f ) );
	newPCIPLabel->SetFontScale( 1.4f );
	newPCIPLabel->SetText( IPString );
	newPCIPLabel->SetTextOffset( Vector3f( 0.0f, 0.0f, 0.01f ) );
	newPCIPLabel->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 550, 120 );

	const int numButtons = 13;
	const char* buttons[numButtons] = {"7","8","9","4","5","6","1","2","3","0",".","<","Enter"};
	int cols = 3;
	newPCIPButtons.resize(numButtons);
	for(int i = 0; i < numButtons; i++ )
	{
		UIButton *button = new UIButton( Cinema.GetGuiSys() );
		button->AddToMenu( Menu, newPCbg );
		button->SetLocalPosition( Vector3f( -0.3f + (i % cols) * 0.3f, 0.45f + (i / cols) * -0.3f, 0.15f ) );
		button->SetText( buttons[i] );
		button->SetLocalScale( Vector3f( 1.0f ) );
		//button->SetFontScale( 1.0f );
		button->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
		//button->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 120, 120 );
        button->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture);
		button->SetOnClick( NewPCIPButtonCallback, this);
		//button->UpdateButtonState();
		newPCIPButtons[i] = button;
	}
	//newPCIPButtons[numButtons - 1]->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, bgTintTexture, 320, 120 );
    newPCIPButtons[numButtons - 1]->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture);
	newPCIPButtons[numButtons - 1]->SetLocalPosition(Vector3f( 0.0f, 0.45f + ( (numButtons -1) / cols) * -0.3f, 0.15f ));

}

void PcSelectionView::NewPCIPButtonPressed( UIButton *button)
{
	VRMenuObject *object = button->GetMenuObject();
	char bLabel = object->GetText()[0];
	//char bLabel = '.';
	int error=0;
	switch(bLabel)
	{
		case '<':
			if( IPoctets[currentOctet] == 0 && currentOctet > 0) currentOctet--;
			IPoctets[currentOctet] /= 10;
			break;
		case '.':
			currentOctet++;
			if(currentOctet > 3) currentOctet = 3;
			break;
		case 'E':
			newPCMenu->SetVisible(false);
			IPString = "";
			for(int i=0;i<=3;i++)
			{
				if( i != 0 ) IPString += ".";
				IPString +=std::to_string( IPoctets[i] ).c_str();
			}
			error = Native::addPCbyIP(IPString.c_str());
			if ( error == 2 )
			{
				SetError( "Error_UnknownHost", false );
			}
			else if( error == 1 )
			{
				SetError( "Error_AddPCFailed", false );
			}
			IPoctets[0] = IPoctets[1] = IPoctets[2] = IPoctets[3] = 0;
			currentOctet = 0;
			break;
		case '0':
			if(IPoctets[currentOctet]==0)
			{
				currentOctet++;
				if(currentOctet > 3) currentOctet = 3;
			}
		default: // numbers
			int number = bLabel - '0';
			if(IPoctets[currentOctet] * 10 + number > 255)
			{
				currentOctet++;
				if(currentOctet > 3) currentOctet = 3;
			}
			if(IPoctets[currentOctet] * 10 + number <= 255)
			{
				IPoctets[currentOctet] = IPoctets[currentOctet] * 10 + number;
			}
			if(IPoctets[currentOctet] >= 26)
			{
				currentOctet++;
				if(currentOctet > 3) currentOctet = 3;
			}
			break;
	}
	IPString = "";
	for(int i=0;i<=3;i++)
	{
		if( i != 0 ) IPString += ".";
		if( i != currentOctet || IPoctets[i] != 0) IPString += std::to_string(IPoctets[i] );
		if( i == currentOctet ) IPString += "_";
	}
	newPCIPLabel->SetText( IPString );

}

void PcSelectionView::CloseAppButtonPressed()
{
	const PcDef* pc = GetSelectedPc();

	if(pc)
	{
		Native::closeApp(pc->UUID.c_str(), 0);
	}
}



Vector3f PcSelectionView::ScalePosition( const Vector3f &startPos, const float scale, const float menuOffset ) const
{
	const float eyeHieght = Cinema.SceneMgr.Scene.GetEyeHeight();

	Vector3f pos = startPos;
	pos.x *= scale;
	pos.y = ( pos.y - eyeHieght ) * scale + eyeHieght + menuOffset;
	pos.z *= scale;
	pos += Cinema.SceneMgr.Scene.GetFootPos();

	return pos;
}

//
// Repositions the menu for the lobby scene or the theater
//
void PcSelectionView::UpdateMenuPosition()
{
	// scale down when in a theater
	const float scale = Cinema.InLobby ? 1.0f : 0.55f;
	CenterRoot->GetMenuObject()->SetLocalScale( Vector3f( scale ) );

	if ( !Cinema.InLobby && Cinema.SceneMgr.SceneInfo.UseFreeScreen )
	{
		Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenPose );
		CenterRoot->GetMenuObject()->SetLocalRotation( orientation );
		CenterRoot->GetMenuObject()->SetLocalPosition( Cinema.SceneMgr.FreeScreenPose.Transform( Vector3f( 0.0f, -1.76f * scale, 0.0f ) ) );
	}
	else
	{
		const float menuOffset = Cinema.InLobby ? 0.0f : 0.5f;
		CenterRoot->GetMenuObject()->SetLocalRotation( Quatf() );
		CenterRoot->GetMenuObject()->SetLocalPosition( ScalePosition( Vector3f::ZERO, scale, menuOffset ) );
	}
}

//============================================================================================

void PcSelectionView::Select()
{
	OVR_LOG( "SelectPc");

	// ignore selection while repositioning screen
	if ( RepositionScreen )
	{
		return;
	}


	PcsIndex = PcBrowser->GetSelection();

	if (PcsIndex >= (int)PcList.size())
	{
		// open up manual IP entry menu
		newPCMenu->SetVisible(true);
		return;
	}


	Cinema.SetPc(PcList[PcsIndex]);


	if ( !Cinema.InLobby )
	{
		Cinema.AppSelection(false);
	}
	else
	{
		Cinema.AppSelection( false );

	}
}

const PcDef *PcSelectionView::GetSelectedPc() const
{
	int selectedItem = PcBrowser->GetSelection();
	if ( ( selectedItem >= 0 ) && ( selectedItem < static_cast< int >(PcList.size()) ) )
	{
		return PcList[ selectedItem ];
	}

	return NULL;
}

void PcSelectionView::SetPcList( const std::vector<const PcDef *> &pcs)
{
	OVR_LOG( "SetPcList: %zu Pcs", pcs.size() );

	PcList = pcs;
	DeletePointerArray( PcBrowserItems );
	for ( OVR::UPInt i = 0; i < PcList.size(); i++ )
	{
		const PcDef *Pc = PcList[ i ];

		OVR_LOG( "AddPc: %s", Pc->Name.c_str() );


		CarouselItem *item = new CarouselItem();
		item->Texture 		= Pc->Poster;
		item->TextureWidth 	= Pc->PosterWidth;
		item->TextureHeight	= Pc->PosterHeight;
		item->UserData 		= ( void * )Pc;
		PcBrowserItems.push_back( item );
	}

	CarouselItem *addPCitem        = new CarouselItem();
	addPCitem->Texture             = newPCTex;
	addPCitem->TextureWidth     = newPCWidth;
	addPCitem->TextureHeight    = newPCHeight;
	addPCitem->UserData = 0;
	PcBrowserItems.push_back( addPCitem );


	PcBrowser->SetItems( PcBrowserItems );

	PcTitle->SetText( "" );
	LastPcDisplayed = NULL;


	if(PcsIndex > static_cast< int >(PcList.size())) PcsIndex = 0;
}

void PcSelectionView::SetCategory( const PcCategory category )
{
    // default to category in index 0
    OVR::UPInt categoryIndex = 0;
    for ( OVR::UPInt i = 0; i < Categories.size(); ++i )
    {
        if ( category == Categories[ i ].Category )
        {
            categoryIndex = i;
            break;
        }
    }

    OVR_LOG( "SetCategory: %s", Categories[ categoryIndex ].Text.c_str() );
    CurrentCategory = Categories[ categoryIndex ].Category;
    for ( OVR::UPInt i = 0; i < Categories.size(); ++i )
    {
        Categories[ i ].Button->SetHilighted( i == categoryIndex );
    }

    // reset all the swipe icons so they match the current poster
    for ( int i = 0; i < NumSwipeTrails; i++ )
    {
        CarouselSwipeHintComponent * compLeft = LeftSwipes[ i ]->GetMenuObject()->GetComponentByTypeName<CarouselSwipeHintComponent>();
        compLeft->Reset( LeftSwipes[ i ]->GetMenuObject() );
        CarouselSwipeHintComponent * compRight = RightSwipes[ i ]->GetMenuObject()->GetComponentByTypeName<CarouselSwipeHintComponent>();
        compRight->Reset( RightSwipes[ i ]->GetMenuObject() );
    }

    SetPcList( Cinema.PcMgr.GetPcList( CurrentCategory ) );

    OVR_LOG( "%zu Pcs added", PcList.size() );
}

void PcSelectionView::UpdatePcTitle()
{
    const PcDef * currentPc = GetSelectedPc();
    if ( LastPcDisplayed != currentPc )
    {
        if ( currentPc != NULL )
        {
            PcTitle->SetText( currentPc->Name.c_str() );
        }
        else
        {

			PcTitle->SetText( Cinema.GetCinemaStrings().title_add_pc );
		}

        LastPcDisplayed = currentPc;
    }
}

void PcSelectionView::SelectionHighlighted( bool isHighlighted )
{
    if ( isHighlighted && !Cinema.InLobby && ( PcsIndex == PcBrowser->GetSelection() ) )
    {
        // dim the poster when the resume icon is up and the poster is highlighted
        CenterPoster->SetColor( Vector4f( 0.55f, 0.55f, 0.55f, 1.0f ) );
    }
    else if ( PcBrowser->HasSelection() )
    {
        CenterPoster->SetColor( Vector4f( 1.0f ) );
    }
}

void PcSelectionView::UpdateSelectionFrame( const ovrApplFrameIn & vrFrame )
{
    const double now = vrapi_GetTimeInSeconds();
    if ( !PcBrowser->HasSelection() )
    {
        SelectionFader.Set( now, 0, now + 0.1, 1.0f );
    }

    if ( !SelectionFrame->GetMenuObject()->IsHilighted() )
    {
        SelectionFader.Set( now, 0, now + 0.1, 1.0f );
    }
    else
    {
        PcBrowser->CheckGamepad( Cinema.GetGuiSys(), vrFrame, PcRoot->GetMenuObject() );
        float touchpadMinSwipe = 100.0f;
        PcBrowser->CheckTouchpad( Cinema.GetGuiSys(), vrFrame,touchpadMinSwipe);
    }

    SelectionFrame->SetColor( Vector4f( static_cast<float>( SelectionFader.Value( now ) ) ) );

	int selected = PcBrowser->GetSelection();
	if ( static_cast< int >(PcList.size()) > selected && PcList[selected]->isRunning )
	{
        ResumeIcon->SetColor( Vector4f( static_cast<float>( SelectionFader.Value( now ) ) ) );
        ResumeIcon->SetTextColor( Vector4f( static_cast<float>( SelectionFader.Value( now ) ) ) );
        ResumeIcon->SetVisible( true );
		CloseAppButton->SetVisible( true );

	}
    else
    {
        ResumeIcon->SetVisible( false );
		CloseAppButton->SetVisible( false );

	}
}

void PcSelectionView::SetError( const char *text, bool showErrorIcon )
{
    ClearError();

    OVR_LOG( "SetError: %s", text );
    if ( showErrorIcon )
    {
        ErrorMessage->SetVisible( true );
        ErrorMessage->SetTextWordWrapped( text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f );
    }
    else
    {
        PlainErrorMessage->SetVisible( true );
        PlainErrorMessage->SetTextWordWrapped( text, Cinema.GetGuiSys().GetDefaultFont(), 1.0f );
    }
    TitleRoot->SetVisible( false );
    PcRoot->SetVisible( false );

    CarouselSwipeHintComponent::ShowSwipeHints = false;
}

void PcSelectionView::ClearError()
{
    OVR_LOG( "ClearError" );
    ErrorMessageClicked = false;
    ErrorMessage->SetVisible( false );
    PlainErrorMessage->SetVisible( false );
    TitleRoot->SetVisible( true );
    PcRoot->SetVisible( true );
	CategoryRoot->SetVisible( true );

    CarouselSwipeHintComponent::ShowSwipeHints = true;
}

bool PcSelectionView::ErrorShown() const
{
    return ErrorMessage->GetVisible() || PlainErrorMessage->GetVisible();
}

void PcSelectionView::Frame( const ovrApplFrameIn & vrFrame )
{/*
    // We want 4x MSAA in the lobby
    ovrEyeBufferParms eyeBufferParms = Cinema.app->GetEyeBufferParms();
    eyeBufferParms.multisamples = 4;
    Cinema.app->SetEyeBufferParms( eyeBufferParms );
*/

    //if ( vrFrame.AllButtons & BUTTON_B )
    {
        /*
        if ( Cinema.InLobby )
        {
            Cinema.app->ShowSystemUI( VRAPI_SYS_UI_CONFIRM_QUIT_MENU );
        }
        else
        {
            Cinema.GetGuiSys().CloseMenu( Menu->GetVRMenu(), false );
        }
         */
    }

    // check if they closed the menu with the back button
    if ( !Cinema.InLobby && Menu->GetVRMenu()->IsClosedOrClosing() && !Menu->GetVRMenu()->IsOpenOrOpening() )
    {
        // if we finished the movie or have an error, don't resume it, go back to the lobby
        if ( ErrorShown() )
        {
            OVR_LOG( "Error closed.  Return to lobby." );
            ClearError();
            Cinema.PcSelection( true );
        }
        else if ( Cinema.IsMovieFinished() )
        {
            OVR_LOG( "Movie finished.  Return to lobby." );
            Cinema.PcSelection( true );
        }
        else
        {
            OVR_LOG( "Resume movie." );
            Cinema.PlayOrResumeOrRestartApp();
        }
    }

    if ( !Cinema.InLobby && ErrorShown() )
    {/*
        CarouselSwipeHintComponent::ShowSwipeHints = false;
        if ( vrFrame.AllButtons & ( BUTTON_TOUCH | BUTTON_A ) )
        {
            Cinema.GetGuiSys().GetSoundEffectPlayer().Play( "touch_down" );
        }
        else if ( vrFrame.LastFrameAllButtons & ( BUTTON_TOUCH | BUTTON_A ) )
        {
            Cinema.GetGuiSys().GetSoundEffectPlayer().Play( "touch_up" );
            ErrorMessageClicked = true;
        }
        else if ( ErrorMessageClicked && ( ( vrFrame.AllButtons & ( BUTTON_TOUCH | BUTTON_A ) ) == 0 ) )
        {
			ClearError();
		}*/
    }

    if ( Cinema.SceneMgr.FreeScreenActive && !ErrorShown() )
    {
        if ( !RepositionScreen && !SelectionFrame->GetMenuObject()->IsHilighted() )
        {
            // outside of screen, so show reposition message
            const double now = vrapi_GetTimeInSeconds();
            float alpha = static_cast<float>( MoveScreenAlpha.Value( now ) );
            if ( alpha > 0.0f )
            {
                MoveScreenLabel->SetVisible( true );
                MoveScreenLabel->SetTextColor( Vector4f( alpha ) );
            }
/*
            if ( vrFrame.AllButtons & ( BUTTON_A | BUTTON_TOUCH ) )
            {
                // disable hit detection on selection frame
                SelectionFrame->GetMenuObject()->AddFlags( VRMENUOBJECT_DONT_HIT_ALL );
                RepositionScreen = true;
            }*/
        }
        else
        {
            // onscreen, so hide message
            const double now = vrapi_GetTimeInSeconds();
            MoveScreenAlpha.Set( now, -1.0f, now + 1.0f, 1.0f );
            MoveScreenLabel->SetVisible( false );
        }

        const Matrix4f invViewMatrix( Cinema.SceneMgr.Scene.GetCenterEyeTransform() );
        const Vector3f viewPos( Cinema.SceneMgr.Scene.GetCenterEyePosition() );
        const Vector3f viewFwd( Cinema.SceneMgr.Scene.GetCenterEyeForward() );

        // spawn directly in front
        Quatf rotation( -viewFwd, 0.0f );
        Quatf viewRot( invViewMatrix );
        Quatf fullRotation = rotation * viewRot;

        const float menuDistance = 1.45f;
        Vector3f position( viewPos + viewFwd * menuDistance );

        MoveScreenLabel->SetLocalPose( fullRotation, position );
    }

    // while we're holding down the button or touchpad, reposition screen
    if ( RepositionScreen )
    {/*
        if ( vrFrame.AllButtons & ( BUTTON_A | BUTTON_TOUCH ) )
        {
            Cinema.SceneMgr.PutScreenInFront();
            Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenPose );
            CenterRoot->GetMenuObject()->SetLocalRotation( orientation );
            CenterRoot->GetMenuObject()->SetLocalPosition( Cinema.SceneMgr.FreeScreenPose.Transform( Vector3f( 0.0f, -1.76f * 0.55f, 0.0f ) ) );

        }
        else*/
        {
            RepositionScreen = false;
        }
    }
    else
    {
        // reenable hit detection on selection frame.
        // note: we do this on the frame following the frame we disabled RepositionScreen on
        // so that the selection object doesn't get the touch up.
        SelectionFrame->GetMenuObject()->RemoveFlags( VRMENUOBJECT_DONT_HIT_ALL );
    }

    UpdatePcTitle();
    UpdateSelectionFrame( vrFrame );
    if (Cinema.PcMgr.updated)
    {
        Cinema.PcMgr.updated = false;
        SetPcList( Cinema.PcMgr.GetPcList( CurrentCategory ) );
    }

    //Cinema.SceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
