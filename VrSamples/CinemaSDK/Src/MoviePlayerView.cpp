/************************************************************************************

Filename    :   MoviePlayerView.cpp
Content     :
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include "CinemaApp.h"
#include "Native.h"
#include "GUI/VRMenuMgr.h"
#include "GUI/GuiSys.h"
#include "System.h"
#include "OVR_Math.h"

#include <sstream>
#include <iomanip>

#include "CinemaStrings.h"

using namespace OVRFW;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Vector4f;
using OVR::Matrix4f;
using OVR::Posef;
using OVR::Quatf;
using OVR::Bounds3f;

namespace OculusCinema
{

const double MoviePlayerView::GazeTimeTimeout = 4;

MoviePlayerView::MoviePlayerView( CinemaApp &cinema ) :
	View( "MoviePlayerView" ),
	Cinema( cinema ),
	uiActive( false ),
	RepositionScreen( false ),
	BackgroundTintTexture(),
	SliderBackgroundTexture(),
	SliderProgressTexture(),
	SliderPosition(),
	MoveScreenAlpha(),
    GazeSlider(),
    TrackpadSlider(),
	BitrateSlider( ),
    DistanceSlider(),
    SizeSlider(),
	settingsVersion(1.0f),
	defaultSettingsPath(""),
	settings1Path(""),
	settings2Path(""),
	settings3Path(""),
	appSettingsPath(""),
	BackgroundClicked( false ),
	UIOpened( false ),
	allowDrag(false),
	mouseMoving(false),
    clickStartTime(0.0),
	lastScroll(0),
	lastMouse(0.0,0.0),
	mouseDownLeft(false),
	mouseDownRight(false),
    mouseDownMiddle(false),
    mouseMode(MOUSE_GAZE),
    gazeScaleValue(1.05),
    trackpadScaleValue(2.0),
	streamAspectRatio(Ratio_16_9),
    streamWidth(1280),
    streamHeight(720),
    streamFPS(60),
    streamHostAudio(true),
	customBitrate(0.0),
	bitrate(0),
	videoSettingsUpdated(false),
	BitrateMin(0.0),
	BitrateMax(20000.0),
	GazeMin(0.7),
	GazeMax(1.58),
	TrackpadMin(-4.0),
	TrackpadMax(4.0),
	VoidScreenDistanceMin(0.1),
	VoidScreenDistanceMax(3.0),
	VoidScreenScaleMin(-3.0),
	VoidScreenScaleMax(4.0),
	oldPoses(),
	calibrationStage(0),
	lastPose(),
	trackCalibrationYaw(500),
	trackCalibrationPitch(500),
	latencyAddition(24),
	screenMotionPaused( false )
{
}

MoviePlayerView::~MoviePlayerView()
{
}

//=========================================================================================

void MoviePlayerView::OneTimeInit( const char * launchIntent )
{
	OVR_LOG( "MoviePlayerView::OneTimeInit" );

	OVR_UNUSED( launchIntent );

	const double start =  GetTimeInSeconds();

	CreateMenu( Cinema.GetGuiSys() );

	OVR_LOG( "MoviePlayerView::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - start );
}

void MoviePlayerView::OneTimeShutdown()
{
	OVR_LOG( "MoviePlayerView::OneTimeShutdown" );
	delete(defaultSettings);
	defaultSettings = NULL;
	delete(appSettings);
	appSettings = NULL;
	delete(settings1);
	settings1 = NULL;
	delete(settings2);
	settings2 = NULL;
	delete(settings3);
	settings3 = NULL;
}

float PixelScale( const float x )
{
	return x * VRMenuObject::DEFAULT_TEXEL_SCALE;
}

Vector3f PixelPos( const float x, const float y, const float z )
{
	return Vector3f( PixelScale( x ), PixelScale( y ), PixelScale( z ) );
}

void MouseMenuButtonCallback		( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->MouseMenuButtonPressed(); }
void StreamMenuButtonCallback		( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->StreamMenuButtonPressed(); }
void ScreenMenuButtonCallback		( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ScreenMenuButtonPressed(); }
void ExitButtonCallback                ( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ExitButtonPressed(); }
void SaveAppCallback				( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->SaveAppPressed(); }
void SaveDefaultCallback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->SaveDefaultPressed(); }
void ResetDefaultCallback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ResetDefaultPressed(); }
void Save1Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Save1Pressed(); }
void Save2Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Save2Pressed(); }
void Save3Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Save3Pressed(); }
void Load1Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Load1Pressed(); }
void Load2Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Load2Pressed(); }
void Load3Callback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Load3Pressed(); }
void GazeCallback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->GazePressed(); }
void TrackpadCallback				( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->TrackpadPressed(); }
void OffCallback					( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->OffPressed(); }
void GazeScaleCallback				( SliderComponent *button, void *object, const float value ) { ( ( MoviePlayerView * )object )->GazeScalePressed( value ); }
void TrackpadScaleCallback			( SliderComponent *button, void *object, const float value ) { ( ( MoviePlayerView * )object )->TrackpadScalePressed( value ); }
void ButtonAspectRatio169Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ButtonAspectRatio169Pressed(); }
void ButtonAspectRatio43Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ButtonAspectRatio43Pressed(); }
void Button4k60Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button4k60Pressed(); }
void Button4k30Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button4k30Pressed(); }
void Button1080p60Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button1080p60Pressed(); }
void Button1080p30Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button1080p30Pressed(); }
void Button720p60Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button720p60Pressed(); }
void Button720p30Callback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->Button720p30Pressed(); }
void HostAudioCallback				( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->HostAudioPressed(); }
void ApplyVideoCallback			( UIButton *button, void *object ) { ( ( MoviePlayerView * )object )->ApplyVideoPressed(); }
void BitrateCallback                ( SliderComponent *button, void *object, const float value ) { ( ( MoviePlayerView * )object )->BitratePressed( value ); }
void DistanceCallback				( SliderComponent *button, void *object, const float value ) { ( ( MoviePlayerView * )object )->DistancePressed( value ); }
void SizeCallback					( SliderComponent *button, void *object, const float value ) { ( ( MoviePlayerView * )object )->SizePressed( value ); }


void MoviePlayerView::TextButtonHelper(UIButton* button, float scale, int w, int h)
{
	button->SetLocalScale( Vector3f( scale ) );
	//button->SET SetFontScale( 1.0f );
	button->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	//button->SetSetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	//button->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
	button->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, w, h );
	button->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );

}


void MoviePlayerView::SetUpSlider(OvrGuiSys & guiSys, UIObject *parent, SliderComponent& scrub, UIImage* bg, UIImage* ind, UILabel* cur, UILabel* set, int slideWidth, int xoff, int yoff)
{
	bg->AddToMenu( PlaybackControlsMenu, parent );
	bg->SetLocalPosition( PixelPos( xoff, yoff, 2 ) );
	bg->SetColor( Vector4f( 0.5333f, 0.5333f, 0.5333f, 1.0f ) );
	bg->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SliderBackgroundTexture, slideWidth + 6, 46 );
	bg->AddComponent( &scrub );

	ind->AddToMenu(  PlaybackControlsMenu, bg );
	ind->SetLocalPosition( PixelPos( 0, 0, 1 ) );
	ind->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SliderProgressTexture, slideWidth, 40 );
	ind->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	cur->AddToMenu( PlaybackControlsMenu, bg );
	cur->SetLocalPosition( PixelPos( 0, 52, 2 ) );
	cur->SetLocalScale( Vector3f( 1.0f ) );
	cur->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SliderPosition );
	cur->SetText( "1.0" );
	cur->SetTextOffset( PixelPos( 0, 6, 1 ) );
	cur->SetFontScale( 0.71f );
	cur->SetColor( Vector4f( 0 / 255.0f, 93 / 255.0f, 219 / 255.0f, 1.0f ) );
	cur->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	cur->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	set->AddToMenu(  PlaybackControlsMenu, bg );
	set->SetLocalPosition( PixelPos( 0, 52, 4 ) );
	set->SetLocalScale( Vector3f( 1.0f ) );
	set->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SliderPosition );
	set->SetText( "1.0" );
	set->SetTextOffset( PixelPos( 0, 6, 1 ) );
	set->SetFontScale( 0.71f );
	set->SetColor( Vector4f( 47.0f / 255.0f, 70 / 255.0f, 89 / 255.0f, 1.0f ) );
	set->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	set->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	scrub.SetWidgets( bg, ind, cur, set, slideWidth );

}

void MoviePlayerView::InitializeSettings()
{
	std::string	outPath;
	const ovrJava & java = *reinterpret_cast< const ovrJava* >( Cinema.GetGuiSys().GetContext()->ContextForVrApi() );
	const bool validDir = ovrFileSys::GetPathIfValidPermission( java, EST_INTERNAL_STORAGE, EFT_FILES, "",  permissionFlags_t( PERMISSION_READ ) | permissionFlags_t( PERMISSION_WRITE ), outPath );

	if(validDir)
	{
		if(defaultSettings == NULL)
		{
			defaultSettingsPath = outPath + "settings.json";
			defaultSettings = new Settings(defaultSettingsPath.c_str());

			defaultSettings->Define("StreamTheaterSettingsVersion", &settingsVersion);
			defaultSettings->Define("MouseMode", (int*)&mouseMode);

			defaultSettings->Define("StreamAspectRatio", (int*)&streamAspectRatio);
			defaultSettings->Define("StreamWidth", &streamWidth);
			defaultSettings->Define("StreamHeight", &streamHeight);
			defaultSettings->Define("StreamFPS", &streamFPS);
			defaultSettings->Define("EnableHostAudio", &streamHostAudio);

			defaultSettings->Define("CustomBitrate", &customBitrate);
			defaultSettings->Define("MinBitrate", &BitrateMin);
			defaultSettings->Define("MaxBitrate", &BitrateMax);


			defaultSettings->Define("GazeScale", &gazeScaleValue);
			defaultSettings->Define("TrackpadScale", &trackpadScaleValue);

			defaultSettings->Define("VoidScreenDistance", &Cinema.SceneMgr.FreeScreenDistance);
			defaultSettings->Define("VoidScreenScale", &Cinema.SceneMgr.FreeScreenScale);

			defaultSettings->Define("GazeScaleMax", &GazeMax);
			defaultSettings->Define("GazeScaleMin", &GazeMin);
			defaultSettings->Define("TrackpadScaleMax", &TrackpadMax);
			defaultSettings->Define("TrackpadScaleMin", &TrackpadMin);
			defaultSettings->Define("VoidScreenDistanceMax", &VoidScreenDistanceMax);
			defaultSettings->Define("VoidScreenDistanceMin", &VoidScreenDistanceMin);
			defaultSettings->Define("VoidScreenScaleMax", &VoidScreenScaleMax);
			defaultSettings->Define("VoidScreenScaleMin", &VoidScreenScaleMin);

			defaultSettings->Load();
		}

		if(settings1 == NULL)
		{
			settings1Path = outPath + "settings.1.json";
			settings2Path = outPath + "settings.2.json";
			settings3Path = outPath + "settings.3.json";

			settings1 = new Settings(settings1Path.c_str());
			settings2 = new Settings(settings2Path.c_str());
			settings3 = new Settings(settings3Path.c_str());

			settings1->CopyDefines(*defaultSettings);
			settings2->CopyDefines(*settings1);
			settings3->CopyDefines(*settings1);
		}

		if(appSettings != NULL)
		{
			delete(appSettings);
			appSettings = NULL;
		}

		appSettingsPath = outPath + "settings." + Cinema.GetCurrentApp()->Name + ".json";
		appSettings = new Settings(appSettingsPath.c_str());
		appSettings->CopyDefines(*defaultSettings);
		appSettings->Load();

		UpdateMenus();
	}
}

void MoviePlayerView::CreateMenu( OvrGuiSys & guiSys )
{
	OVR_UNUSED( guiSys );

	BackgroundTintTexture.LoadTextureFromApplicationPackage( "assets/backgroundTint.png" );

	SliderBackgroundTexture.LoadTextureFromApplicationPackage( "assets/img_seekbar_background.png" );
	SliderProgressTexture.LoadTextureFromApplicationPackage( "assets/img_seekbar_progress_blue.png" );

	SliderPosition.LoadTextureFromApplicationPackage( "assets/img_seek_position.png" );

	MouseTexture.LoadTextureFromApplicationPackage( "assets/mousebutton.png" );
	MouseHoverTexture.LoadTextureFromApplicationPackage( "assets/mousebutton.png" );
	MousePressedTexture.LoadTextureFromApplicationPackage( "assets/mousebutton.png" );

	StreamTexture.LoadTextureFromApplicationPackage( "assets/streambutton.png" );
	StreamHoverTexture.LoadTextureFromApplicationPackage( "assets/streambutton.png" );
	StreamPressedTexture.LoadTextureFromApplicationPackage( "assets/streambutton.png" );

	ScreenTexture.LoadTextureFromApplicationPackage( "assets/screenbutton.png" );
	ScreenHoverTexture.LoadTextureFromApplicationPackage( "assets/screenbutton.png" );
	ScreenPressedTexture.LoadTextureFromApplicationPackage( "assets/screenbutton.png" );

	ExitTexture.LoadTextureFromApplicationPackage( "assets/exitbutton.png" );
	ExitHoverTexture.LoadTextureFromApplicationPackage( "assets/exitbutton.png" );
	ExitPressedTexture.LoadTextureFromApplicationPackage( "assets/exitbutton.png" );

	ButtonTexture.LoadTextureFromApplicationPackage( "assets/button.png" );
	ButtonHoverTexture.LoadTextureFromApplicationPackage( "assets/button_hoover.png" );
	ButtonPressedTexture.LoadTextureFromApplicationPackage( "assets/button_pressed.png" );


	// ==============================================================================
    //
    // reorient message
    //
	MoveScreenMenu = new UIMenu( Cinema.GetGuiSys() );
	MoveScreenMenu->Create( "MoviePlayerMenu" );
	MoveScreenMenu->SetFlags( VRMenuFlags_t( VRMENU_FLAG_TRACK_GAZE ) | VRMenuFlags_t( VRMENU_FLAG_BACK_KEY_DOESNT_EXIT ) );

	MoveScreenLabel=new UILabel( Cinema.GetGuiSys() );
	MoveScreenLabel->AddToMenu( MoveScreenMenu, NULL );
    MoveScreenLabel->SetLocalPose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, -1.8f ) );
    MoveScreenLabel->GetMenuObject()->AddFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
    MoveScreenLabel->SetFontScale( 0.5f );
    MoveScreenLabel->SetText( Cinema.GetCinemaStrings().MoviePlayer_Reorient );
    MoveScreenLabel->SetTextOffset( Vector3f( 0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );  // offset to be below gaze cursor
    MoveScreenLabel->SetVisible( false );

    // ==============================================================================
    //
    // Playback controls
    //
    PlaybackControlsMenu = new UIMenu( Cinema.GetGuiSys() );
    PlaybackControlsMenu->Create( "PlaybackControlsMenu" );
    PlaybackControlsMenu->SetFlags( VRMenuFlags_t( VRMENU_FLAG_BACK_KEY_DOESNT_EXIT ) );

	PlaybackControlsPosition=new UIContainer( Cinema.GetGuiSys() );
    PlaybackControlsPosition->AddToMenu( PlaybackControlsMenu );
	PlaybackControlsScale=new UIContainer( Cinema.GetGuiSys() );
    PlaybackControlsScale->AddToMenu( PlaybackControlsMenu, PlaybackControlsPosition );
    PlaybackControlsScale->SetLocalPosition( Vector3f( 0.0f, 0.0f, 0.01f ) );
    PlaybackControlsScale->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 1080, 1080 );

	// ==============================================================================
    //
    // movie title
    //
	MovieTitleLabel=new UILabel( Cinema.GetGuiSys() );
    MovieTitleLabel->AddToMenu( PlaybackControlsMenu, PlaybackControlsScale );
    MovieTitleLabel->SetLocalPosition( PixelPos( 0, 266, 0 ) );
    MovieTitleLabel->SetFontScale( 1.4f );
    MovieTitleLabel->SetText( "" );
    MovieTitleLabel->SetTextOffset( Vector3f( 0.0f, 0.0f, 0.01f ) );
    MovieTitleLabel->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 320, 120 );

    // ==============================================================================
    //
    // controls
    //
	ControlsBackground=new UIImage( Cinema.GetGuiSys() );
    ControlsBackground->AddToMenu( PlaybackControlsMenu, PlaybackControlsScale );
	ControlsBackground->AddFlags( VRMENUOBJECT_RENDER_HIERARCHY_ORDER );

	ControlsBackground->SetLocalPosition( PixelPos( 220, 500, 0 ) );

	ControlsBackground->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 1200, 168 );
    ControlsBackground->AddComponent( GazeTimer = new ControlsGazeTimer() );

	MouseMenuButton=new UIButton( Cinema.GetGuiSys() );
	MouseMenuButton->AddToMenu(  PlaybackControlsMenu, ControlsBackground );
	MouseMenuButton->SetLocalScale( Vector3f( 2.0f ) );
	MouseMenuButton->SetButtonImages( MouseTexture, MouseHoverTexture, MousePressedTexture );
	MouseMenuButton->SetOnClick( MouseMenuButtonCallback, this );
	MouseMenuButton->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );

	StreamMenuButton=new UIButton( Cinema.GetGuiSys()  );
	StreamMenuButton->AddToMenu(  PlaybackControlsMenu, ControlsBackground );
	StreamMenuButton->SetLocalScale( Vector3f( 2.0f ) );
	StreamMenuButton->SetButtonImages( StreamTexture, StreamHoverTexture, StreamPressedTexture );
	StreamMenuButton->SetOnClick( StreamMenuButtonCallback, this );
	StreamMenuButton->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );

	ScreenMenuButton=new UIButton( Cinema.GetGuiSys()  );
	ScreenMenuButton->AddToMenu(  PlaybackControlsMenu, ControlsBackground );
	ScreenMenuButton->SetLocalScale( Vector3f( 2.0f ) );
	ScreenMenuButton->SetButtonImages( ScreenTexture, ScreenHoverTexture, ScreenPressedTexture );
	ScreenMenuButton->SetOnClick( ScreenMenuButtonCallback, this );
	ScreenMenuButton->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );

	ExitButton=new UIButton( Cinema.GetGuiSys() );
	ExitButton->AddToMenu( PlaybackControlsMenu, ControlsBackground );
	ExitButton->SetLocalScale( Vector3f( 2.0f ) );
	ExitButton->SetButtonImages( ExitTexture, ExitHoverTexture, ExitPressedTexture );
	ExitButton->SetOnClick( ExitButtonCallback, this );
	ExitButton->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );

	static const int MenuButtonsWidth = 1100;
	static const int MenuButtonsCount = 6;
	static const int MenuButtonsStep = MenuButtonsWidth / ( MenuButtonsCount - 1 );
	int menuButtonPos = 0 - MenuButtonsWidth / 2;

	MouseMenuButton->SetLocalPosition( PixelPos( menuButtonPos, 0, 1 ) );
	menuButtonPos += MenuButtonsStep;
	StreamMenuButton->SetLocalPosition( PixelPos( menuButtonPos, 0, 1 ) );
	menuButtonPos += MenuButtonsStep;
	ScreenMenuButton->SetLocalPosition( PixelPos( menuButtonPos, 0, 1 ) );
	menuButtonPos += MenuButtonsStep;
	ExitButton->SetLocalPosition( PixelPos( menuButtonPos, 0, 1 ) );


	const static int MENU_X = 200;
	const static int MENU_Y = -150;
	const static int MENU_TOP = 200;

    const static int MENU_DIST_1_ROW= 0;
    const static int MENU_DIST_2_ROW= 85;
    const static int MENU_DIST_3_ROW= 170;


	SaveMenu = new UIContainer( Cinema.GetGuiSys() );
	SaveMenu->AddToMenu(  PlaybackControlsMenu, PlaybackControlsScale );
	SaveMenu->SetLocalPosition( PixelPos( 0, -450, 1 ) );
	SaveMenu->SetVisible(false);

	ButtonSaveApp=new UIButton( Cinema.GetGuiSys());
	ButtonSaveApp->AddToMenu( PlaybackControlsMenu, SaveMenu );
	ButtonSaveApp->SetLocalPosition( PixelPos( 660, MENU_DIST_3_ROW, 1 ) );
	ButtonSaveApp->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveApp.c_str()  );
	ButtonSaveApp->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonSaveApp, 0.5f);
	ButtonSaveApp->SetOnClick( SaveAppCallback, this);

	ButtonSaveDefault=new UIButton( Cinema.GetGuiSys() );
	ButtonSaveDefault->AddToMenu(  PlaybackControlsMenu, SaveMenu );
	ButtonSaveDefault->SetLocalPosition( PixelPos( 660, MENU_DIST_2_ROW, 1 ) );
	ButtonSaveDefault->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveDefault.c_str()  );
	ButtonSaveDefault->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonSaveDefault, 0.5f);
	ButtonSaveDefault->SetOnClick( SaveDefaultCallback, this);

	ButtonResetSettings=new UIButton( Cinema.GetGuiSys() );
	ButtonResetSettings->AddToMenu(  PlaybackControlsMenu, SaveMenu );
	ButtonResetSettings->SetLocalPosition( PixelPos( 660, MENU_DIST_1_ROW, 1 ) );
	ButtonResetSettings->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonResetSettings.c_str()  );
	ButtonResetSettings->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonResetSettings, 0.5f);
	ButtonResetSettings->SetOnClick( ResetDefaultCallback, this);

	ButtonSaveSettings1=new UIButton( Cinema.GetGuiSys() );
	ButtonSaveSettings1->AddToMenu( PlaybackControlsMenu, SaveMenu );
	ButtonSaveSettings1->SetLocalPosition( PixelPos( 0, MENU_DIST_2_ROW, 1 ) );
	ButtonSaveSettings1->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveSettings1.c_str()  );
	ButtonSaveSettings1->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonSaveSettings1, 0.5f);
	ButtonSaveSettings1->SetOnClick( Save1Callback, this);

	ButtonSaveSettings2=new UIButton( Cinema.GetGuiSys() );
	ButtonSaveSettings2->AddToMenu( PlaybackControlsMenu, SaveMenu );
	ButtonSaveSettings2->SetLocalPosition( PixelPos( 220, MENU_DIST_2_ROW, 1 ) );
	ButtonSaveSettings2->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveSettings2.c_str()  );
	ButtonSaveSettings2->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonSaveSettings2, 0.5f);
	ButtonSaveSettings2->SetOnClick( Save2Callback, this);

	ButtonSaveSettings3=new UIButton( Cinema.GetGuiSys() );
	ButtonSaveSettings3->AddToMenu( PlaybackControlsMenu, SaveMenu );
	ButtonSaveSettings3->SetLocalPosition( PixelPos( 440, MENU_DIST_2_ROW, 1 ) );
	ButtonSaveSettings3->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSaveSettings3.c_str()  );
	ButtonSaveSettings3->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonSaveSettings3, 0.5f);
	ButtonSaveSettings3->SetOnClick( Save3Callback, this);

	ButtonLoadSettings1=new UIButton( Cinema.GetGuiSys() );
	ButtonLoadSettings1->AddToMenu(  PlaybackControlsMenu, SaveMenu );
	ButtonLoadSettings1->SetLocalPosition( PixelPos( 0, MENU_DIST_1_ROW, 1 ) );
	ButtonLoadSettings1->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonLoadSettings1.c_str()  );
	ButtonLoadSettings1->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonLoadSettings1, 0.5f);
	ButtonLoadSettings1->SetOnClick( Load1Callback, this);

	ButtonLoadSettings2=new UIButton( Cinema.GetGuiSys() );
	ButtonLoadSettings2->AddToMenu(  PlaybackControlsMenu, SaveMenu );
	ButtonLoadSettings2->SetLocalPosition( PixelPos( 220, MENU_DIST_1_ROW, 1 ) );
	ButtonLoadSettings2->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonLoadSettings2.c_str() );
	ButtonLoadSettings2->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonLoadSettings2, 0.5f);
	ButtonLoadSettings2->SetOnClick( Load2Callback, this);

	ButtonLoadSettings3=new UIButton( Cinema.GetGuiSys() );
	ButtonLoadSettings3->AddToMenu(  PlaybackControlsMenu, SaveMenu );
	ButtonLoadSettings3->SetLocalPosition( PixelPos( 440, MENU_DIST_1_ROW, 1 ) );
	ButtonLoadSettings3->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonLoadSettings3.c_str() );
	ButtonLoadSettings3->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	TextButtonHelper(ButtonLoadSettings3, 0.5f);
	ButtonLoadSettings3->SetOnClick( Load3Callback, this);


	MouseMenu = new UIContainer( Cinema.GetGuiSys() );
	MouseMenu->AddToMenu(  PlaybackControlsMenu, PlaybackControlsScale );
	MouseMenu->SetLocalPosition( PixelPos( 0, MENU_TOP, 1 ) );
	MouseMenu->SetVisible(false);


	ButtonGaze=new UIButton( Cinema.GetGuiSys()  );
	ButtonGaze->AddToMenu(  PlaybackControlsMenu, MouseMenu );
	ButtonGaze->SetLocalPosition( PixelPos( MENU_X * -1.8, MENU_Y * 1, 1 ) );
	ButtonGaze->SetText(  Cinema.GetCinemaStrings().ButtonText_ButtonGaze.c_str());
	TextButtonHelper(ButtonGaze);
	ButtonGaze->SetOnClick( GazeCallback, this);
    ButtonGaze->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
    ButtonGaze->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonTrackpad=new UIButton (Cinema.GetGuiSys() );
	ButtonTrackpad->AddToMenu(  PlaybackControlsMenu, MouseMenu );
	ButtonTrackpad->SetLocalPosition( PixelPos( MENU_X * 0, MENU_Y * 1, 1 ) );
	ButtonTrackpad->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonTrackpad.c_str() );
	TextButtonHelper(ButtonTrackpad);
	ButtonTrackpad->SetOnClick( TrackpadCallback, this);
    ButtonTrackpad->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	ButtonTrackpad->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonOff=new UIButton( Cinema.GetGuiSys() );
    ButtonOff->AddToMenu(  PlaybackControlsMenu, MouseMenu );
	ButtonOff->SetLocalPosition( PixelPos( MENU_X * 1.8, MENU_Y * 1, 1 ) );
	ButtonOff->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonOff.c_str()  );
	TextButtonHelper(ButtonOff);
	ButtonOff->SetOnClick( OffCallback, this);
    ButtonOff->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	ButtonOff->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	GazeScale=new UILabel( Cinema.GetGuiSys() );
	GazeScale->AddToMenu( PlaybackControlsMenu, MouseMenu );
	GazeScale->SetLocalPosition( PixelPos( MENU_X * -1, MENU_Y * 2, 1 ) );
    GazeScale->SetText( Cinema.GetCinemaStrings().ButtonText_LabelGazeScale.c_str()  );


	GazeScale->SetLocalScale( Vector3f( 1.0f ) );
	GazeScale->SetFontScale( 1.0f );
	GazeScale->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	GazeScale->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	GazeScale->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 300, 80 );
	GazeSliderBackground=new UIImage( Cinema.GetGuiSys() );
	GazeSliderIndicator=new UIImage( Cinema.GetGuiSys() );
	GazeCurrentSetting=new UILabel( Cinema.GetGuiSys() );
	GazeNewSetting=new UILabel( Cinema.GetGuiSys() );
	SetUpSlider(guiSys, MouseMenu, GazeSlider, GazeSliderBackground, GazeSliderIndicator, GazeCurrentSetting, GazeNewSetting, 300, MENU_X * -1, MENU_Y * 3);
	GazeSlider.SetOnClick( GazeScaleCallback, this );

	TrackpadScale=new UILabel( Cinema.GetGuiSys() );
	TrackpadScale->AddToMenu(  PlaybackControlsMenu, MouseMenu );
	TrackpadScale->SetLocalPosition( PixelPos( MENU_X * 1, MENU_Y * 2, 1 ) );
    TrackpadScale->SetText( Cinema.GetCinemaStrings().ButtonText_LabelTrackpadScale.c_str()  );
	TrackpadScale->SetLocalScale( Vector3f( 1.0f ) );
	TrackpadScale->SetFontScale( 1.0f );
	TrackpadScale->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	TrackpadScale->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	TrackpadScale->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 300, 80 );
	TrackpadSliderBackground=new UIImage( Cinema.GetGuiSys() );
	TrackpadSliderIndicator=new UIImage( Cinema.GetGuiSys() );
	TrackpadCurrentSetting=new UILabel( Cinema.GetGuiSys() );
	TrackpadNewSetting=new UILabel( Cinema.GetGuiSys() );
	SetUpSlider(guiSys, MouseMenu, TrackpadSlider, TrackpadSliderBackground, TrackpadSliderIndicator, TrackpadCurrentSetting, TrackpadNewSetting, 300,  MENU_X * 1, MENU_Y * 3);
	TrackpadSlider.SetOnClick( TrackpadScaleCallback, this );


	StreamMenu = new UIContainer( Cinema.GetGuiSys() );
	StreamMenu->AddToMenu(  PlaybackControlsMenu, PlaybackControlsScale );
	StreamMenu->SetLocalPosition( PixelPos( 0, MENU_TOP, 1 ) );
	StreamMenu->SetVisible(false);

	Button4k60=new UIButton( Cinema.GetGuiSys()  );
    Button4k60->AddToMenu(  PlaybackControlsMenu, StreamMenu );
    Button4k60->SetLocalPosition( PixelPos( MENU_X * -1, MENU_Y * 1, 1 ) );
    Button4k60->SetText( Cinema.GetCinemaStrings().ButtonText_Button4k60.c_str() );
    TextButtonHelper(Button4k60);
    Button4k60->SetOnClick( Button4k60Callback, this);
    Button4k60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button4k60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	Button4k30=new UIButton( Cinema.GetGuiSys()  );
    Button4k30->AddToMenu( PlaybackControlsMenu, StreamMenu );
    Button4k30->SetLocalPosition( PixelPos( MENU_X * 1, MENU_Y * 1, 1 ) );
    Button4k30->SetText( Cinema.GetCinemaStrings().ButtonText_Button4k30.c_str() );
    TextButtonHelper(Button4k30);
    Button4k30->SetOnClick( Button4k30Callback, this);
    Button4k30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button4k30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonAspectRatio169=new UIButton( Cinema.GetGuiSys()  );
	ButtonAspectRatio169->AddToMenu( PlaybackControlsMenu, StreamMenu );
	ButtonAspectRatio169->SetLocalPosition( PixelPos( MENU_X * 3 , MENU_Y * 1, 1 ) );
	ButtonAspectRatio169->SetText( "16:9" );
	TextButtonHelper(ButtonAspectRatio169);
	ButtonAspectRatio169->SetOnClick( ButtonAspectRatio169Callback, this);
	ButtonAspectRatio169->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	ButtonAspectRatio169->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	Button1080p60=new UIButton( Cinema.GetGuiSys()  );
	Button1080p60->AddToMenu(  PlaybackControlsMenu, StreamMenu );
	Button1080p60->SetLocalPosition( PixelPos( MENU_X * -1, MENU_Y * 2, 1 ) );
	Button1080p60->SetText( Cinema.GetCinemaStrings().ButtonText_Button1080p60.c_str() );
	TextButtonHelper(Button1080p60);
	Button1080p60->SetOnClick( Button1080p60Callback, this);
    Button1080p60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button1080p60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	Button1080p30=new UIButton( Cinema.GetGuiSys()  );
    Button1080p30->AddToMenu( PlaybackControlsMenu, StreamMenu );
	Button1080p30->SetLocalPosition( PixelPos( MENU_X * 1, MENU_Y * 2, 1 ) );
	Button1080p30->SetText( Cinema.GetCinemaStrings().ButtonText_Button1080p30.c_str() );
	TextButtonHelper(Button1080p30);
	Button1080p30->SetOnClick( Button1080p30Callback, this);
    Button1080p30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button1080p30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonAspectRatio43=new UIButton( Cinema.GetGuiSys()  );
	ButtonAspectRatio43->AddToMenu( PlaybackControlsMenu, StreamMenu );
	ButtonAspectRatio43->SetLocalPosition( PixelPos( MENU_X * 3 , MENU_Y * 2, 1 ) );
	ButtonAspectRatio43->SetText( "4:3" );
	TextButtonHelper(ButtonAspectRatio43);
	ButtonAspectRatio43->SetOnClick( ButtonAspectRatio43Callback, this);
	ButtonAspectRatio43->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	ButtonAspectRatio43->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	Button720p60=new UIButton( Cinema.GetGuiSys()  );
    Button720p60->AddToMenu(  PlaybackControlsMenu, StreamMenu );
	Button720p60->SetLocalPosition( PixelPos( MENU_X * -1, MENU_Y * 3, 1 ) );
	Button720p60->SetText( Cinema.GetCinemaStrings().ButtonText_Button720p60.c_str()  );
	TextButtonHelper(Button720p60);
	Button720p60->SetOnClick( Button720p60Callback, this);
    Button720p60->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button720p60->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	Button720p30=new UIButton( Cinema.GetGuiSys() );
    Button720p30->AddToMenu( PlaybackControlsMenu, StreamMenu );
	Button720p30->SetLocalPosition( PixelPos( MENU_X * 1, MENU_Y * 3, 1 ) );
	Button720p30->SetText( Cinema.GetCinemaStrings().ButtonText_Button720p30.c_str()  );
	TextButtonHelper(Button720p30);
	Button720p30->SetOnClick( Button720p30Callback, this);
    Button720p30->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	Button720p30->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonHostAudio=new UIButton( Cinema.GetGuiSys()  );
	ButtonHostAudio->AddToMenu(  PlaybackControlsMenu, StreamMenu );
	ButtonHostAudio->SetLocalPosition( PixelPos( MENU_X * -1, MENU_Y * 4, 1 ) );
	ButtonHostAudio->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonHostAudio.c_str());
	TextButtonHelper(ButtonHostAudio);
	ButtonHostAudio->SetOnClick( HostAudioCallback, this);
    ButtonHostAudio->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	ButtonHostAudio->SetAsToggleButton(ButtonPressedTexture,Vector4f( 1.0f ));

	ButtonApply=new UIButton( Cinema.GetGuiSys()  );
	ButtonApply->AddToMenu(  PlaybackControlsMenu, StreamMenu );
	ButtonApply->SetLocalPosition( PixelPos( MENU_X * -2.7, MENU_Y * 2, 1 ) );
	ButtonApply->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonApply.c_str() );
	TextButtonHelper(ButtonApply);
	ButtonApply->SetOnClick( ApplyVideoCallback, this);
    ButtonApply->SetButtonImages( ButtonTexture, ButtonHoverTexture, ButtonPressedTexture );
	//ButtonApply->SetIsEnabled( ApplyVideoIsEnabledCallback, this);

	BitrateAdjust=new UILabel( Cinema.GetGuiSys()  );
	BitrateAdjust->AddToMenu( PlaybackControlsMenu, StreamMenu );
	BitrateAdjust->SetLocalPosition( PixelPos( MENU_X * -2.7, MENU_Y * 2.75, 1 ) );
	BitrateAdjust->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonBitrate.c_str() );
	BitrateAdjust->SetLocalScale( Vector3f( 1.0f ) );
	BitrateAdjust->SetFontScale( 1.0f );
	BitrateAdjust->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	BitrateAdjust->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	BitrateAdjust->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 240, 80 );
	BitrateSliderBackground=new UIImage( Cinema.GetGuiSys()   );
	BitrateSliderIndicator=new UIImage( Cinema.GetGuiSys()  );
	BitrateCurrentSetting=new UILabel( Cinema.GetGuiSys()  );
	BitrateNewSetting=new UILabel( Cinema.GetGuiSys()  );
	SetUpSlider(guiSys, StreamMenu, BitrateSlider, BitrateSliderBackground, BitrateSliderIndicator, BitrateCurrentSetting, BitrateNewSetting, 300, MENU_X * -2.7, MENU_Y * 3.5);
	BitrateSlider.SetOnClick( BitrateCallback, this );

    ScreenMenu = new UIContainer( Cinema.GetGuiSys() );
    ScreenMenu->AddToMenu(  PlaybackControlsMenu, PlaybackControlsScale );
    ScreenMenu->SetLocalPosition( PixelPos( 0, MENU_TOP, 1 ) );
    ScreenMenu->SetVisible(false);

	ScreenDistance=new UILabel( Cinema.GetGuiSys() );
	ScreenDistance->AddToMenu(  PlaybackControlsMenu, ScreenMenu );
	ScreenDistance->SetLocalPosition( PixelPos( MENU_X * -0.5, MENU_Y * 1.25, 1 ) );
    ScreenDistance->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonDistance.c_str() );
	ScreenDistance->SetLocalScale( Vector3f( 1.0f ) );
	ScreenDistance->SetFontScale( 1.0f );
	ScreenDistance->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	ScreenDistance->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	ScreenDistance->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 240, 80 );
	//ScreenDistance->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );
	DistanceSliderBackground=new UIImage( Cinema.GetGuiSys() );
	DistanceSliderIndicator=new UIImage( Cinema.GetGuiSys() );
	DistanceCurrentSetting=new UILabel( Cinema.GetGuiSys() );
	DistanceNewSetting=new UILabel( Cinema.GetGuiSys() );
	SetUpSlider(guiSys, ScreenMenu, DistanceSlider, DistanceSliderBackground, DistanceSliderIndicator, DistanceCurrentSetting, DistanceNewSetting, 800, MENU_X * 2, MENU_Y * 1.25);
	DistanceSlider.SetOnClick( DistanceCallback, this );


	ScreenSize=new UILabel( Cinema.GetGuiSys() );
	ScreenSize->AddToMenu(  PlaybackControlsMenu, ScreenMenu );
	ScreenSize->SetLocalPosition( PixelPos( MENU_X * -0.5, MENU_Y * 2.25 , 1 ) );
    ScreenSize->SetText( Cinema.GetCinemaStrings().ButtonText_ButtonSize.c_str() );
	ScreenSize->SetLocalScale( Vector3f( 1.0f ) );
	ScreenSize->SetFontScale( 1.0f );
	ScreenSize->SetColor( Vector4f( 0.0f, 0.0f, 0.0f, 1.0f ) );
	ScreenSize->SetTextColor( Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );
	ScreenSize->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 240, 80 );
	//ScreenSize->GetMenuObject()->SetLocalBoundsExpand( PixelPos( 20, 0, 0 ), Vector3f::ZERO );
	SizeSliderBackground=new UIImage( Cinema.GetGuiSys() );
	SizeSliderIndicator=new UIImage( Cinema.GetGuiSys() );
	SizeCurrentSetting=new UILabel( Cinema.GetGuiSys() );
	SizeNewSetting=new UILabel( Cinema.GetGuiSys() );
	SetUpSlider(guiSys, ScreenMenu, SizeSlider, SizeSliderBackground, SizeSliderIndicator, SizeCurrentSetting, SizeNewSetting, 800, MENU_X * 2, MENU_Y * 2.25);
	SizeSlider.SetOnClick( SizeCallback, this );
}

void MoviePlayerView::OnOpen( const double currTimeInSeconds )
{
	OVR_LOG( "OnOpen" );
	CurViewState = VIEWSTATE_OPEN;

	Cinema.SceneMgr.ClearMovie();

	RepositionScreen = false;
	MoveScreenAlpha.Set( 0, 0, 0, 0.0f );
	InitializeSettings();
	HideUI();

    GazeSlider.SetExtents(GazeMax,GazeMin,2);
    GazeSlider.SetValue(gazeScaleValue);
    TrackpadSlider.SetExtents(TrackpadMax,TrackpadMin,2);
    TrackpadSlider.SetValue(trackpadScaleValue);
	BitrateSlider.SetExtents(BitrateMax,BitrateMin,-1);
	BitrateSlider.SetValue(customBitrate);
	DistanceSlider.SetExtents(VoidScreenDistanceMax,VoidScreenDistanceMin,2);
    DistanceSlider.SetValue(Cinema.SceneMgr.FreeScreenDistance);
    SizeSlider.SetExtents(VoidScreenScaleMax,VoidScreenScaleMin,2);
    SizeSlider.SetValue(Cinema.SceneMgr.FreeScreenScale);

	Cinema.SceneMgr.LightsOff( 1.5f ,currTimeInSeconds);

    Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate);


	MovieTitleLabel->SetText( Cinema.GetCurrentApp()->Name.c_str() );
	Bounds3f titleBounds = MovieTitleLabel->GetTextLocalBounds( Cinema.GetGuiSys().GetDefaultFont() ) * VRMenuObject::TEXELS_PER_METER;
	MovieTitleLabel->SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, titleBounds.GetSize().x + 88, titleBounds.GetSize().y + 32 );
}

void MoviePlayerView::OnClose()
{
	OVR_LOG( "OnClose" );
	CurViewState = VIEWSTATE_CLOSED;
	HideUI();
	Cinema.GetGuiSys().GetGazeCursor().ShowCursor();

	if ( MoveScreenMenu->IsOpen() )
	{
		MoveScreenLabel->SetVisible( false );
		MoveScreenMenu->Close();
	}

	Cinema.SceneMgr.ClearMovie();

	if ( Cinema.SceneMgr.VoidedScene )
	{
		Cinema.SceneMgr.SetSceneModel( Cinema.GetCurrentTheater() );
		Cinema.SceneMgr.PutScreenInFront();
	}
}

void MoviePlayerView::EnteredVrMode()
{
	OVR_LOG( "EnteredVrMode" );
	Cinema.AppSelection( false );
}

void MoviePlayerView::LeavingVrMode()
{
	OVR_LOG( "LeavingVrMode" );
	Native::StopMovie( );
}

void MoviePlayerView::SetError( const char *text, bool showErrorIcon )
{
    OVR_LOG( "Error: %s", text );
    Cinema.AppSelection( true );
    Cinema.ShowError( text );
}

void MoviePlayerView::BackPressed()
{
	OVR_LOG( "BackPressed" );
	if( screenMotionPaused )
	{
		screenMotionPaused = false;
		return;
	}


	if(uiActive)
    {
        HideUI();
    }
    else
    {
        ShowUI(vrapi_GetTimeInSeconds());
    }
}
void MoviePlayerView::BackPressedDouble()
{
    OVR_LOG( "BackPressed" );
    HideUI();

	if ( Cinema.AllowTheaterSelection() )
	{
        OVR_LOG( "Opening TheaterSelection" );
        Cinema.TheaterSelection();
    }
    else
    {
        OVR_LOG( "Opening MovieSelection" );
        Cinema.AppSelection( true );
    }
}

bool MoviePlayerView::OnKeyEvent( const int keyCode, const int repeatCount, const UIKeyboard::KeyEventType eventType )
{
	OVR_UNUSED( repeatCount );

	switch ( keyCode )
	{/*
		case OVR_KEY_BACK:
		{
			switch ( eventType )
			{
				case KEY_EVENT_SHORT_PRESS:
                    OVR_LOG( "KEY_EVENT_SHORT_PRESS" );
                    BackPressed();
                    return true;
                    break;
                default:
				    //OVR_LOG( "unexpected back key state %i", eventType );
				    break;
			}
		}
		break;
		case OVR_KEY_MENU:
		{
			switch ( eventType )
			{
				case KEY_EVENT_SHORT_PRESS:
					OVR_LOG( "KEY_EVENT_SHORT_PRESS" );
					BackPressed();
					return true;
					break;
				default:
					//OVR_LOG( "unexpected back key state %i", eventType );
					break;
			}
		}
		break;*/
	}
	return false;
}

//=========================================================================================

static bool InsideUnit( const Vector2f v )
{
	return v.x > -1.0f && v.x < 1.0f && v.y > -1.0f && v.y < 1.0f;
}

void MoviePlayerView::ShowUI( const double currTimeInSeconds )
{
	OVR_LOG( "ShowUI" );
	Cinema.GetGuiSys().GetGazeCursor().ShowCursor();

	PlaybackControlsMenu->Open();
	GazeTimer->SetGazeTime(currTimeInSeconds);
	MovieTitleLabel->SetVisible(true);

	PlaybackControlsScale->SetLocalScale( Vector3f( Cinema.SceneMgr.GetScreenSize().y * ( 500.0f / 1080.0f ) ) );
	PlaybackControlsPosition->SetLocalPose( Cinema.SceneMgr.GetScreenPose() );

	uiActive = true;
}

void MoviePlayerView::HideUI()
{
	OVR_LOG( "HideUI" );
	SaveMenu->SetVisible(false);
    MouseMenu->SetVisible(false);
    ScreenMenu->SetVisible(false);
    StreamMenu->SetVisible(false);
	PlaybackControlsMenu->Close();

	Cinema.GetGuiSys().GetGazeCursor().HideCursor();
	uiActive = false;

	BackgroundClicked = false;
}

void MoviePlayerView::CheckDebugControls( const ovrApplFrameIn & vrFrame )
{
	if ( !Cinema.AllowDebugControls )
	{
		return;
	}
/*
	if (  vrFrame.AllButtons | BUTTON_X )
	{
		//Cinema.SceneMgr.ToggleLights( 0.5f );
	}

	// Press Y to toggle FreeScreen mode, while holding the scale and distance can be adjusted
	if (  vrFrame.AllButtons | BUTTON_SELECT)
	{
		Cinema.SceneMgr.FreeScreenActive = !Cinema.SceneMgr.FreeScreenActive || Cinema.SceneMgr.SceneInfo.UseFreeScreen;
		Cinema.SceneMgr.PutScreenInFront();
	}

	if ( Cinema.SceneMgr.FreeScreenActive && (  vrFrame.AllButtons | BUTTON_SELECT ) )
	{
		Cinema.SceneMgr.FreeScreenDistance -= vrFrame.GamePad.LeftJoystick.y * vrFrame.DeltaSeconds * 3;
		Cinema.SceneMgr.FreeScreenDistance = clamp< float >( Cinema.SceneMgr.FreeScreenDistance, 1.0f, 4.0f );
		Cinema.SceneMgr.FreeScreenScale += vrFrame.GamePad.LeftJoystick.x * vrFrame.DeltaSeconds * 3;
		Cinema.SceneMgr.FreeScreenScale = clamp< float >( Cinema.SceneMgr.FreeScreenScale, 1.0f, 4.0f );
	}*/
}

static Vector3f	MatrixOrigin( const Matrix4f & m )
{
	return Vector3f( -m.M[0][3], -m.M[1][3], -m.M[2][3] );
}

static Vector3f	MatrixForward( const Matrix4f & m )
{
	return Vector3f( -m.M[2][0], -m.M[2][1], -m.M[2][2] );
}

// -1 to 1 range on screenMatrix, returns -2,-2 if looking away from the screen
Vector2f MoviePlayerView::GazeCoordinatesOnScreen( const Matrix4f & viewMatrix, const Matrix4f screenMatrix ) const
{
	// project along -Z in the viewMatrix onto the Z = 0 plane of screenMatrix
	const Vector3f viewForward = MatrixForward( viewMatrix ).Normalized();

	Vector3f screenForward;
	if (1 || Cinema.SceneMgr.FreeScreenActive )
	{
		// FIXME: free screen matrix is inverted compared to bounds screen matrix.
		screenForward = -Vector3f( screenMatrix.M[0][2], screenMatrix.M[1][2], screenMatrix.M[2][2] ).Normalized();
	}
	else
	{
		screenForward = -MatrixForward( screenMatrix ).Normalized();
	}

	const float approach = viewForward.Dot( screenForward );
	if ( approach <= 0.1f )
	{
		// looking away
		return Vector2f( -2.0f, -2.0f );
	}

	const Matrix4f panelInvert = screenMatrix.Inverted();
	const Matrix4f viewInvert = viewMatrix.Inverted();

	const Vector3f viewOrigin = viewInvert.Transform( Vector3f( 0.0f ) );
	const Vector3f panelOrigin = MatrixOrigin( screenMatrix );

	// Should we disallow using panels from behind?
	const float d = panelOrigin.Dot( screenForward );
	const float t = -( viewOrigin.Dot( screenForward ) + d ) / approach;

	const Vector3f impact = viewOrigin + viewForward * t;
	const Vector3f localCoordinate = panelInvert.Transform( impact );

	return Vector2f( localCoordinate.x, localCoordinate.y );
}


Matrix4f MoviePlayerView::InterpolatePoseAtTime( long time )
{//faire un iterateur de liste
	OldScreenPose before = oldPoses.front();
	OldScreenPose after = oldPoses.front();
	std::list<OldScreenPose>::iterator i = oldPoses.begin();
	while (after.time < time && i != oldPoses.end())
	{
		i++;
		OldScreenPose tooOld = before;
		before = after;
		after = *i;
		if(tooOld != before) {
			std::advance(i,-2);
			i = oldPoses.erase(i);//remove tooOld
		}
	}

/*
	OldScreenPose before = oldPoses.front();
	OldScreenPose after = oldPoses.front();
	while(after.time < time && after!=oldPoses.back())
	{
		OldScreenPose tooOld = before;
		before = after;
		after = oldPoses.GetNext(after);
		if(tooOld != before) {
			oldPoses.remove(tooOld);
		}
	}
*/
//    if(before->time == after->time)
	{
		return after.pose;
	}
	return ( before.pose * (time - before.time)
			 + after.pose * (after.time - time) )
		   / (after.time - before.time);
}

void MoviePlayerView::RecordPose( long time, Matrix4f pose )
{
	OldScreenPose p;
	p.time = time;
	p.pose = pose;
	oldPoses.push_back(p);
}

void MoviePlayerView::HandleCalibration( const  ovrApplFrameIn & vrFrame )
{
	switch(calibrationStage) {
		case -1: // TODO: Calibration was canceled, display a temporary message
			break;
		case 1: // Look up
			break;
		case 2: // Look down
			break;
		case 3: // Look center
			break;
		case 4: // Look at a recognizable object
			break;
		case 5: // Spin in a complete circle and look at the object again
			break;
		default:
			calibrationStage = -1;
			break;
	}

}



// This gets called right after the screen is updated by the scene manager
// to minimize the number of times where we have the wrong timestamp
// This gets called right after the screen is updated by the scene manager
// to minimize the number of times where we have the wrong timestamp
void MoviePlayerView::MovieScreenUpdated()
{

}

#define SCROLL_CLICKS 8
    void MoviePlayerView::HandleGazeMouse( const ovrApplFrameIn & vrFrame, bool onscreen, const Vector2f screenCursor )
    {
        if(onscreen) {
            Vector2f travel = screenCursor - lastMouse;
            if(travel.x != 0.0 && travel.y != 0.0)
            {
				Native::MouseMove( streamWidth / 2 * gazeScaleValue * travel.x, (streamHeight / -2 * gazeScaleValue * travel.y ));
			}
            lastMouse = screenCursor;
        }
/*
        // Left click
        if ( ( vrFrame.LastFrameAllButtons & BUTTON_TOUCH ))
        {
            if(onscreen) {
                if(allowDrag && !mouseDownLeft)
                {
                    Native::MouseClick(1,true); // fast click!
                }
                Native::MouseClick(1,false);
                //Cinema.app->PlaySound( "touch_up" );
                mouseDownLeft = false;
            } else		// open ui if it's not visible
            {
                ShowUI(vrFrame.RealTimeInSeconds);
                // ignore button A or touchpad until release so we don't close the UI immediately after opening it
                UIOpened = true;
            }
        }

        if ( onscreen && ( vrFrame.AllButtons & BUTTON_TOUCH ) ) {
            clickStartTime = vrapi_GetTimeInSeconds();
            allowDrag = true;
        }
        */
/*
        if( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) {
            allowDrag = false;
        }
*/
/*
        if ( onscreen && allowDrag && !mouseDownLeft && (clickStartTime + 0.5 < vrapi_GetTimeInSeconds()) &&
             (  vrFrame.AllButtons & BUTTON_TOUCH ) )
        {
            Native::MouseClick(1,true);
            //Cinema.app->PlaySound( "touch_down" );
            mouseDownLeft = true;
            allowDrag = false; // already dragging
        }

        // Right click
        if ( onscreen && !mouseDownRight && (  vrFrame.AllButtons & BUTTON_SWIPE_BACK ) )
        {
            Native::MouseClick(3,true);
            //Cinema.app->PlaySound( "touch_down" );
            mouseDownRight = true;
        }

        // Middle click
        if ( onscreen && !mouseDownMiddle && (  vrFrame.AllButtons & BUTTON_SWIPE_FORWARD ) )
        {
            Native::MouseClick(2,true);
            //Cinema.app->PlaySound( "touch_down" );
            mouseDownMiddle = true;
        }

        // Mouse Scroll
        // It says it's 0.0-1.0, but it's calculated as if(d>100) swf = d/100, so it can't be less than 1.0.
        float actualSwipeFraction =0;// vrFrame.Input.swipeFraction - 1.0;
        if ( onscreen && ( vrFrame.AllTouches & BUTTON_SWIPE_UP ) )
        {
            signed char diff = (signed char)(SCROLL_CLICKS * actualSwipeFraction) - lastScroll;
            //LOG("Scroll up %i %f", diff, vrFrame.Input.swipeFraction);
            lastScroll = (signed char)(SCROLL_CLICKS * actualSwipeFraction);
            if(diff)
            {
                Native::MouseScroll(diff );
                //Cinema.app->PlaySound( "touch_up" );
            }
        }
        if ( onscreen && (vrFrame.AllTouches & BUTTON_SWIPE_DOWN ) )
        {
            signed char diff = lastScroll - (signed char)(SCROLL_CLICKS * actualSwipeFraction);
            //LOG("Scroll down %i %f", diff, vrFrame.Input.swipeFraction);
            lastScroll = (signed char)(SCROLL_CLICKS * actualSwipeFraction);
            if(diff)
            {
                Native::MouseScroll(diff );
                //Cinema.app->PlaySound( "touch_down" );
            }
        }

        if (  vrFrame.LastFrameAllButtons & BUTTON_TOUCH )
        {
            if(mouseDownRight)
            {
                Native::MouseClick(3,false);
                mouseDownRight = false;
                //Cinema.app->PlaySound( "touch_up" );
            }
            if(mouseDownMiddle)
            {
                Native::MouseClick(2,false);
                mouseDownMiddle = false;
                //Cinema.app->PlaySound( "touch_up" );
            }
            if(mouseDownLeft)
            {
                Native::MouseClick(1,false);
                mouseDownLeft = false;
                //Cinema.app->PlaySound( "touch_up" );
            }

            lastScroll = 0;
        }*/
    }

    void MoviePlayerView::HandleTrackpadMouse( const ovrApplFrameIn & vrFrame )
    {/*
        if (  vrFrame.AllButtons & BUTTON_TOUCH ) {
            clickStartTime = vrapi_GetTimeInSeconds();
        }*/
/*
        if( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) {
            if(mouseMoving)
            {
                Vector2f travel = vrFrame.Input.touchRelative - lastMouse;
                lastMouse = vrFrame.Input.touchRelative;
				Native::MouseMove( - travel.x * trackpadScaleValue, travel.y * trackpadScaleValue);
			}
            else
            {
                lastMouse = vrFrame.Input.touchRelative;
                mouseMoving = true;
            }
        }
*/
/*
        if ( !mouseMoving && (clickStartTime + 0.5 < vrapi_GetTimeInSeconds()) &&
             ( vrFrame.AllTouches & BUTTON_TOUCH ) )
        {
            Native::MouseClick(3,true);
            //Cinema.app->PlaySound( "touch_down" );
            mouseDownRight = true;
        }

        if (  vrFrame.LastFrameAllButtons & BUTTON_TOUCH )
        {
            if( !mouseDownLeft && !mouseDownRight  )
            {
                Native::MouseClick(1,true); // fast click!
                Native::MouseClick(1,false);
            }
            if( mouseDownRight )
            {
                Native::MouseClick(3,false);
            }
            if(mouseDownLeft)
            {
                Native::MouseClick(1,false);
            }
            //Cinema.app->PlaySound( "touch_up" );
            mouseDownLeft = false;
            mouseDownRight = false;
            mouseMoving = false;
        }*/
    }



//#define SCROLL_CLICKS 8
void MoviePlayerView::CheckInput( const ovrApplFrameIn & vrFrame )
{
	// while we're holding down the button or touchpad, reposition screen
	if ( RepositionScreen ) {/*
		if ( vrFrame.AllTouches & BUTTON_TOUCH ) {
			Cinema.SceneMgr.PutScreenInFront();
		} else */{
			RepositionScreen = false;
		}
		return;
	}

	const Vector2f screenCursor = GazeCoordinatesOnScreen( Cinema.SceneMgr.Scene.GetCenterEyeViewMatrix(), Cinema.SceneMgr.ScreenMatrix() );
	bool onscreen = false;

	if ( Cinema.HeadsetWasMounted() )
	{
		Cinema.AppSelection( false );
	}
	else if ( Cinema.HeadsetWasUnmounted() )
	{
	}

	if ( InsideUnit( screenCursor ) )
	{
		onscreen = true;
	}
	else if ( uiActive )
	{
		onscreen = GazeTimer->IsFocused();
	}
/*
    OVR_LOG("vrFrame.Input.NumKeyEvents i %i", vrFrame.Input.NumKeyEvents);
    for (int i=0; i< vrFrame.Input.NumKeyEvents; i++ ){
        OVR_LOG("keyevent i, code, %i %i=", i, vrFrame.Input.KeyEvents[i].KeyCode);
    }
*/


    /*if (    vrFrame.AllButtons & BUTTON_TOUCH_DOUBLE){
        if(uiActive)
        {
            HideUI();
        }
        else
        {
            ShowUI();
        }
    }*/

    if( ( !RepositionScreen ) && ( !uiActive ) && mouseMode != MOUSE_OFF )
    {
        if( mouseMode == MOUSE_GAZE)
        {
            HandleGazeMouse(vrFrame, onscreen, screenCursor);
        }
        else if( mouseMode == MOUSE_TRACKPAD)
        {
            HandleTrackpadMouse(vrFrame);
        }
    }
    else
    {/*
        // Left click
        if ( (  vrFrame.LastFrameAllButtons & BUTTON_TOUCH )  )
        {
            ShowUI(vrFrame.RealTimeInSeconds);
            // ignore button A or touchpad until release so we don't close the UI immediately after opening it
            UIOpened = true;
        }*/
    }

	/*if(onscreen) {
		if(lastMouse.x >= -1 && lastMouse.x <= 1 && lastMouse.y >= -1 && lastMouse.y <= 1)
		{
			Vector2f travel = screenCursor - lastMouse;
			Native::MouseMove(Cinema.app, 1280 / 2 * 1.2 * travel.x, 720 / -2 * 1.2 * travel.y );
		}

		lastMouse = screenCursor;

	}



	if ( ( vrFrame.Input.buttonReleased & BUTTON_TOUCH ) )
	{
		// open ui if it's not visible
		//Cinema.app->PlaySound( "touch_up" );
		if(onscreen) {
			if(allowDrag && !mouseDownLeft)
			{
				Native::MouseClick(1,true); // fast click!
			}
			Native::MouseClick(1,false);
			//Cinema.app->PlaySound( "touch_up" );
			mouseDownLeft = false;
		} else        // open ui if it's not visible
		{
			ShowUI();
			UIOpened = true;
		}
		// ignore button A or touchpad until release so we don't close the UI immediately after opening it
		UIOpened = true;
	}

	if ( onscreen && (  vrFrame.AllButtons & BUTTON_TOUCH ) ) {
		clickStartTime = vrapi_GetTimeInSeconds();
		allowDrag = true;
	}

	if( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) {
		allowDrag = false;

	}

	if ( onscreen && allowDrag && !mouseDownLeft && (clickStartTime + 0.5 < vrapi_GetTimeInSeconds()) &&
			( vrFrame.Input.buttonState & BUTTON_TOUCH ) && !( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) )
	{
		Native::MouseClick(1,true);
		//Cinema.app->PlaySound( "touch_down" );
		mouseDownLeft = true;
		allowDrag = false; // already dragging
	}


	// Right click
	if ( onscreen && !mouseDownRight && (  vrFrame.AllButtons & BUTTON_SWIPE_BACK ) )
	{
		Native::MouseClick(3,true);
		//Cinema.app->PlaySound( "touch_down" );
		mouseDownRight = true;

	}

	// Middle click
	if ( onscreen && !mouseDownMiddle && (  vrFrame.AllButtons & BUTTON_SWIPE_FORWARD ) )
	{
		Native::MouseClick(2,true);
		//Cinema.app->PlaySound( "touch_up" );
		mouseDownMiddle = true;

	}

	// Mouse Scroll
	// It says it's 0.0-1.0, but it's calculated as if(d>100) swf = d/100, so it can't be less than 1.0.
	float actualSwipeFraction = vrFrame.Input.swipeFraction - 1.0;
	if ( onscreen && ( vrFrame.Input.buttonState & BUTTON_SWIPE_UP ) )
	{
		signed char diff = (signed char)(SCROLL_CLICKS * actualSwipeFraction) - lastScroll;
		//LOG("Scroll up %i %f", diff, vrFrame.Input.swipeFraction);
		lastScroll = (signed char)(SCROLL_CLICKS * actualSwipeFraction);
		if(diff)
		{
			Native::MouseScroll(Cinema.app, diff );
			//Cinema.app->PlaySound( "touch_up" );
		}

	}
	if ( onscreen && ( vrFrame.Input.buttonState & BUTTON_SWIPE_DOWN ) )
	{
		signed char diff = lastScroll - (signed char)(SCROLL_CLICKS * actualSwipeFraction);
		//LOG("Scroll down %i %f", diff, vrFrame.Input.swipeFraction);
		lastScroll = (signed char)(SCROLL_CLICKS * actualSwipeFraction);
		if(diff)
		{
			Native::MouseScroll(Cinema.app, diff );
			//Cinema.app->PlaySound( "touch_down" );
		}

	}

	if ( vrFrame.Input.buttonReleased & BUTTON_TOUCH )
	{
		if(mouseDownRight)
		{
			Native::MouseClick(3,false);
			mouseDownRight = false;
			//Cinema.app->PlaySound( "touch_up" );
		}
		if(mouseDownMiddle)
		{
			Native::MouseClick(2,false);
			mouseDownMiddle = false;
			//Cinema.app->PlaySound( "touch_up" );
		}
		if(mouseDownLeft)
		{
			Native::MouseClick(1,false);
			mouseDownLeft = false;
			//Cinema.app->PlaySound( "touch_up" );
		}
		lastScroll = 0;
	}
*/

	if ( Cinema.SceneMgr.FreeScreenActive )
	{
		const Vector2f screenCursor = GazeCoordinatesOnScreen( Cinema.SceneMgr.Scene.GetCenterEyeViewMatrix(), Cinema.SceneMgr.ScreenMatrix() );
		bool onscreen = false;
		if ( InsideUnit( screenCursor ) )
		{
			onscreen = true;
		}
		else if ( uiActive )
		{
			onscreen = GazeTimer->IsFocused();
		}

		if ( !onscreen )
		{
			// outside of screen, so show reposition message
			const double now = vrFrame.RealTimeInSeconds;
			float alpha = static_cast<float>( MoveScreenAlpha.Value( now ) );
			if ( alpha > 0.0f )
			{
				MoveScreenLabel->SetVisible( true );
				MoveScreenLabel->SetTextColor( Vector4f( alpha ) );
			}
/*
			if (  vrFrame.AllButtons &  BUTTON_TOUCH  )
			{
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
	}


	/*if (  vrFrame.AllButtons & BUTTON_START )
	{
		
	}

	if (  vrFrame.AllButtons & BUTTON_SELECT )
	{
		// movie select
		Cinema.GetGuiSys().GetSoundEffectPlayer().Play( "touch_up" );
		Cinema.AppSelection( false );
	}

	if (  vrFrame.AllButtons & BUTTON_B )
	{
		if ( !uiActive )
		{
			BackPressed();
		}
		else
		{
			OVR_LOG( "User pressed button 2" );
			Cinema.GetGuiSys().GetSoundEffectPlayer().Play( "touch_up" );
			HideUI();
		}
	}*/
}


void MoviePlayerView::MouseMenuButtonPressed()
{

	//Cinema.app->PlaySound( "touch_up" );
	UpdateMenus();
	MouseMenu->SetVisible(!MouseMenu->GetVisible());
	StreamMenu->SetVisible(false);
	ScreenMenu->SetVisible(false);
	//ControllerMenu->SetVisible(false);
	SaveMenu->SetVisible(MouseMenu->GetVisible());
}
void MoviePlayerView::StreamMenuButtonPressed()
{

	//Cinema.app->PlaySound( "touch_up" );
	UpdateMenus();
	MouseMenu->SetVisible(false);
	StreamMenu->SetVisible(!StreamMenu->GetVisible());
	ScreenMenu->SetVisible(false);
	//ControllerMenu->SetVisible(false);
	SaveMenu->SetVisible(StreamMenu->GetVisible());
}
void MoviePlayerView::ScreenMenuButtonPressed()
{

	//Cinema.app->PlaySound( "touch_up" );
	UpdateMenus();
	MouseMenu->SetVisible(false);
	StreamMenu->SetVisible(false);
	ScreenMenu->SetVisible(!ScreenMenu->GetVisible());
	//ControllerMenu->SetVisible(false);
	SaveMenu->SetVisible(ScreenMenu->GetVisible());
}

void MoviePlayerView::ExitButtonPressed()
{
	HideUI();
	if ( Cinema.AllowTheaterSelection() )
	{
		OVR_LOG( "Opening TheaterSelection" );
		Cinema.TheaterSelection();
	}
	else
	{
		OVR_LOG( "Opening MovieSelection" );
		Cinema.AppSelection( true );
	}
}

void MoviePlayerView::SaveAppPressed()
{
	if(!appSettings) return;
	appSettings->SaveChanged();
	UpdateMenus();
}
void MoviePlayerView::SaveDefaultPressed()
{
	if(!defaultSettings) return;
//	defaultSettings->SaveChanged();
//	defaultSettings->SaveVarNames();
	defaultSettings->SaveAll();
	UpdateMenus();
}
void MoviePlayerView::ResetDefaultPressed()
{
	OVR_LOG("Resetting default settings");
	remove(defaultSettingsPath.c_str());
	delete(defaultSettings);
	defaultSettings = NULL;

	OVR_LOG("Resetting settings 1");
	remove(settings1Path.c_str());
	delete(settings1);
	settings1 = NULL;

	OVR_LOG("Resetting settings 2");
	remove(settings2Path.c_str());
	delete(settings2);
	settings2 = NULL;

	OVR_LOG("Resetting settings 3");
	remove(settings3Path.c_str());
	delete(settings3);
	settings3 = NULL;

	OVR_LOG("Resetting app settings");
	remove(appSettingsPath.c_str());
	delete(appSettings);
	appSettings = NULL;

	//FIXME: Define all these defaults in a better place
	gazeScaleValue = 1.05;
	trackpadScaleValue = 2.0;
	streamWidth = 1280;
	streamHeight = 720;
	streamAspectRatio = Ratio_16_9;
	streamFPS = 60;
	streamHostAudio = true;
	GazeMin = 0.7;
	GazeMax = 1.58;
	TrackpadMin = -4.0;
	TrackpadMax = 4.0;
	VoidScreenDistanceMin = 0.1;
	VoidScreenDistanceMax = 3.0;
	VoidScreenScaleMin = -3.0;
	VoidScreenScaleMax = 4.0;
	Cinema.SceneMgr.FreeScreenDistance = 1.5;
	Cinema.SceneMgr.FreeScreenScale = 1.0;

	InitializeSettings();

	Cinema.SceneMgr.ClearMovie();
	UpdateMenus();
	Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio,bitrate);

}
void MoviePlayerView::Save1Pressed()
{
	if(!settings1) return;
	settings1->SaveAll();
	UpdateMenus();
}
void MoviePlayerView::Save2Pressed()
{
	if(!settings2) return;
	settings2->SaveAll();
	UpdateMenus();
}
void MoviePlayerView::Save3Pressed()
{
	if(!settings3) return;
	settings3->SaveAll();
	UpdateMenus();
}
void MoviePlayerView::Load1Pressed()
{
	LoadSettings(settings1);
}
void MoviePlayerView::Load2Pressed()
{
	LoadSettings(settings2);
}
void MoviePlayerView::Load3Pressed()
{
	LoadSettings(settings3);
}
void MoviePlayerView::LoadSettings(Settings* set)
{
	int oldWidth = 	streamWidth;
	int oldHeight = streamHeight;
	int oldFPS = streamFPS;
	int oldAspectRatio = streamAspectRatio;

	set->Load();

	if( oldWidth != streamWidth || oldHeight != streamHeight || oldFPS != streamFPS || oldAspectRatio != streamAspectRatio)
	{
		Cinema.SceneMgr.ClearMovie();
		Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate);
	}

	GazeSlider.SetExtents(GazeMax,GazeMin,2);
	GazeSlider.SetValue(gazeScaleValue);
	TrackpadSlider.SetExtents(TrackpadMax,TrackpadMin,2);
	TrackpadSlider.SetValue(trackpadScaleValue);
	BitrateSlider.SetExtents(BitrateMax,BitrateMin,-1);
	BitrateSlider.SetValue(customBitrate);
	DistanceSlider.SetExtents(VoidScreenDistanceMax,VoidScreenDistanceMin,2);
	DistanceSlider.SetValue(Cinema.SceneMgr.FreeScreenDistance);
	SizeSlider.SetExtents(VoidScreenScaleMax,VoidScreenScaleMin,2);
	SizeSlider.SetValue(Cinema.SceneMgr.FreeScreenScale);

	UpdateMenus();
}

int MoviePlayerView::WidthByAspect(){

	if (streamAspectRatio == Ratio_16_9){
		return streamWidth;
	}else{
		return ((streamHeight/3)*4);
	}


}

// Mouse controls
void MoviePlayerView::GazePressed()
{
	mouseMode = MOUSE_GAZE;
	ButtonGaze->SetHilighted(true);
	UpdateMenus();
}
void MoviePlayerView::TrackpadPressed()
{
	mouseMode = MOUSE_TRACKPAD;
	UpdateMenus();
}
void MoviePlayerView::OffPressed()
{
	mouseMode = MOUSE_OFF;
	UpdateMenus();
}

void MoviePlayerView::GazeScalePressed(const float value)
{
	gazeScaleValue = value;
	GazeSlider.SetValue( value );
}
void MoviePlayerView::TrackpadScalePressed(const float value)
{
	trackpadScaleValue = value;
	TrackpadSlider.SetValue( value );
}

//Aspect ratio controls


void MoviePlayerView::ButtonAspectRatio169Pressed()
{
	Cinema.SceneMgr.ClearMovie();
	streamAspectRatio = Ratio_16_9;
	UpdateMenus();
	//TODO: This all needs to be saved in prefs
	Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate);
}

void MoviePlayerView::ButtonAspectRatio43Pressed()
{
	Cinema.SceneMgr.ClearMovie();
	streamAspectRatio = Ratio_4_3;
	UpdateMenus();
	//TODO: This all needs to be saved in prefs
	Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate );
}

// Stream controls

void MoviePlayerView::Button4k60Pressed()
{
	Cinema.SceneMgr.ClearMovie();
	//TODO: This all needs to be saved in prefs
	streamWidth = 3840;
	streamHeight = 2160;
	streamFPS = 60;
	UpdateMenus();
	Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate);
}
void MoviePlayerView::Button4k30Pressed()
{
	Cinema.SceneMgr.ClearMovie();
	streamWidth = 3840;
	streamHeight = 2160;
	streamFPS = 30;
	UpdateMenus();
	Cinema.StartMoviePlayback(WidthByAspect(), streamHeight, streamFPS, streamHostAudio, bitrate);
}
void MoviePlayerView::Button1080p60Pressed()
{
	//Cinema.SceneMgr.ClearMovie();
	//TODO: This all needs to be saved in prefs
	streamWidth = 1920;
	streamHeight = 1080;
	videoSettingsUpdated = true;
	UpdateMenus();
}
void MoviePlayerView::Button1080p30Pressed()
{
	streamWidth = 1920;
	streamHeight = 1080;
	videoSettingsUpdated = true;
	UpdateMenus();

}
void MoviePlayerView::Button720p60Pressed()
{

	streamWidth = 1280;
	streamHeight = 720;
	videoSettingsUpdated = true;
	UpdateMenus();

}
void MoviePlayerView::Button720p30Pressed()
{
	streamWidth = 1280;
	streamHeight = 720;
	videoSettingsUpdated = true;
	UpdateMenus();

}

void MoviePlayerView::HostAudioPressed()
{

	streamHostAudio = !streamHostAudio;
	videoSettingsUpdated = true;
	UpdateMenus();
}


void MoviePlayerView::BitratePressed( const float value)
{
	customBitrate = value;

	bitrate = customBitrate;
	if ( bitrate <= ( BitrateMin + BitrateMax * 0.05  ) )
	{
		bitrate = 0;
		customBitrate = 0.0;
		BitrateSlider.SetValue( 0 );
	}
	else
	{
		BitrateSlider.SetValue( value );
	}

	if(videoSettingsUpdated == false)
	{
		videoSettingsUpdated = true;
		UpdateMenus();
	}
}
void MoviePlayerView::ApplyVideoPressed()
{
	videoSettingsUpdated = false;
	Cinema.SceneMgr.ClearMovie();
	UpdateMenus();
	Cinema.StartMoviePlayback(streamWidth, streamHeight, streamFPS, streamHostAudio, bitrate);
}

bool MoviePlayerView::ApplyVideoIsEnabled()
{
	return videoSettingsUpdated;
}


// Screen controls
void MoviePlayerView::DistancePressed( const float value)
{
	Cinema.SceneMgr.FreeScreenDistance =  value;
	DistanceSlider.SetValue( value );
}
void MoviePlayerView::SizePressed( const float value)
{
	Cinema.SceneMgr.FreeScreenScale = value;
	SizeSlider.SetValue( value );
}


void MoviePlayerView::UpdateMenus()
{
	ButtonGaze->UpdateButtonState();
	ButtonTrackpad->UpdateButtonState();
	ButtonOff->UpdateButtonState();

	ButtonAspectRatio169->UpdateButtonState();
	ButtonAspectRatio43->UpdateButtonState();

	Button4k60->UpdateButtonState();
	Button4k30->UpdateButtonState();
	Button1080p60->UpdateButtonState();
	Button1080p30->UpdateButtonState();
	Button720p60->UpdateButtonState();
	Button720p30->UpdateButtonState();
	//ButtonHostAudio->UpdateButtonState();
	ButtonApply->UpdateButtonState();
}


void MoviePlayerView::UpdateUI( const ovrApplFrameIn & vrFrame )
{
	if ( uiActive )
	{
        ////TODO: Commenting this out for now until I have a better idea of how to handle UI interaction.
        //// The GazeTimer is currently only attached to the main menu buttons, so this code breaks the submenu options
        //// Either it gazetimer needs to cover everything, it needs to be refactored to a function call, or deleted
        //        double timeSinceLastGaze = vrapi_GetTimeInSeconds() - GazeTimer.GetLastGazeTime();
        //        if ( !ScrubBar.IsScrubbing() && ( timeSinceLastGaze > GazeTimeTimeout ) )
        //        {
        //            OVR_LOG( "Gaze timeout" );
        //            HideUI();
        //        }


        // if we press the touchpad or a button outside of the playback controls, then close the UI
	/*	if ( (  vrFrame.AllButtons & BUTTON_TOUCH ) != 0 )
    		{
			// ignore button A or touchpad until release so we don't close the UI immediately after opening it

        if ( !GazeTimer.IsFocused() && BackgroundClicked )
			{
				OVR_LOG( "Clicked outside playback controls" );
				Cinema.GetGuiSys().GetSoundEffectPlayer().Play( "touch_up" );
				HideUI();
			}
			BackgroundClicked = false;
		}

	*/

		if ( Cinema.SceneMgr.FreeScreenActive )
		{
			// update the screen position & size;
			PlaybackControlsScale->SetLocalScale( Vector3f( Cinema.SceneMgr.GetScreenSize().y * ( 500.0f / 1080.0f ) ) );
			PlaybackControlsPosition->SetLocalPose( Cinema.SceneMgr.GetScreenPose() );
		}
	}

	// clear the flag for ignoring button A or touchpad until release
	UIOpened = false;
}

/*
 * Frame()
 *
 * App override
 */
void MoviePlayerView::Frame( const ovrApplFrameIn & vrFrame )
{/*
	// Drop to 2x MSAA during playback, people should be focused
	// on the high quality screen.
	ovrEyeBufferParms eyeBufferParms = Cinema.app->GetEyeBufferParms();
	eyeBufferParms.multisamples = 2;
	Cinema.app->SetEyeBufferParms( eyeBufferParms );
*/
	if ( Native::HadPlaybackError( ) )
	{
		OVR_LOG( "Playback failed" );
		Cinema.UnableToPlayMovie();
	}
	else if ( Native::IsPlaybackFinished( ) )
	{
		OVR_LOG( "Playback finished" );
		Cinema.MovieFinished();
	}

	CheckInput( vrFrame );
	CheckDebugControls( vrFrame );
	UpdateUI( vrFrame );

	if ( Cinema.SceneMgr.FreeScreenActive && !MoveScreenMenu->IsOpen() )
	{
		MoveScreenMenu->Open();
	}
	else if ( !Cinema.SceneMgr.FreeScreenActive && MoveScreenMenu->IsOpen() )
	{
		MoveScreenMenu->Close();
	}

	//Cinema.SceneMgr.Frame( vrFrame );
}

/*************************************************************************************/

ControlsGazeTimer::ControlsGazeTimer() :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) |
			VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST ),
    LastGazeTime( 0 ),
    HasFocus( false )

{
}

void ControlsGazeTimer::SetGazeTime( const double currTimeInSeconds )
{
	LastGazeTime = currTimeInSeconds;
}

eMsgStatus ControlsGazeTimer::OnEvent_Impl( OvrGuiSys & guiSys, ovrApplFrameIn const & vrFrame,
        VRMenuObject * self, VRMenuEvent const & event )
{
	OVR_UNUSED( guiSys );
	OVR_UNUSED( vrFrame );
	OVR_UNUSED( self );

    switch( event.EventType )
    {
    	case VRMENU_EVENT_FRAME_UPDATE:
    		if ( HasFocus )
    		{
    			LastGazeTime = vrFrame.RealTimeInSeconds;
    		}
    		return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_FOCUS_GAINED:
        	HasFocus = true;
        	LastGazeTime = vrFrame.RealTimeInSeconds;
    		return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_FOCUS_LOST:
        	HasFocus = false;
    		return MSG_STATUS_ALIVE;
        default:
            OVR_ASSERT( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

/*************************************************************************************/
SliderComponent::SliderComponent():
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
		VRMENU_EVENT_TOUCH_DOWN |
		VRMENU_EVENT_FRAME_UPDATE |
		VRMENU_EVENT_FOCUS_GAINED |
        VRMENU_EVENT_FOCUS_LOST ),
	HasFocus( false ),
	TouchDown( false ),
	Progress( 0.0f ),
	Max(1.0),
	Min(0.0),
	Background( NULL ),
	ScrubBar( NULL ),
	CurrentTime( NULL ),
	SeekTime( NULL ),
	OnClickFunction( NULL ),
	OnClickObject( NULL )

{
}

    void SliderComponent::SetExtents( const float max, const float min, const int sigfigs )
    {
        Max = max;
        Min = min;
        SigFigs = sigfigs;

        SetProgress( Progress );
    }

    float SliderComponent::ScaleValue(const float value)
    {
        return Min + (Max - Min) * value;
    }

    void SliderComponent::SetOnClick( void ( *callback )( SliderComponent *, void *, float ), void *object )
    {
        OnClickFunction = callback;
        OnClickObject = object;
    }

    void SliderComponent::SetWidgets( UIObject *background, UIObject *scrubBar, UILabel *currentTime, UILabel *seekTime, const int scrubBarWidth )
    {
        Background 		= background;
        ScrubBar 		= scrubBar;
        CurrentTime 	= currentTime;
        SeekTime 		= seekTime;
        ScrubBarWidth	= scrubBarWidth;

        SeekTime->SetVisible( false );
    }

    void SliderComponent::SetValue( const float value )
    {
        float prog = (value - Min) / (Max - Min) ;
        if(prog>1.0) prog = 1.0;
        if(prog<0.0) prog = 0.0;
        SetProgress( prog );
    }

    void SliderComponent::SetProgress( const float progress )
    {
        Progress = progress;
        const float seekwidth = ScrubBarWidth * progress;

        Vector3f pos = ScrubBar->GetLocalPosition();
        pos.x = PixelScale( ( ScrubBarWidth - seekwidth ) * -0.5f );
        ScrubBar->SetLocalPosition( pos );
        ScrubBar->SetSurfaceDims( 0, Vector2f( seekwidth, 40.0f ) );
        ScrubBar->RegenerateSurfaceGeometry( 0, false );

        pos = CurrentTime->GetLocalPosition();
        pos.x = PixelScale( ScrubBarWidth * -0.5f + seekwidth );
        CurrentTime->SetLocalPosition( pos );
        SetText( CurrentTime, ScaleValue(progress) );
    }

    void SliderComponent::SetText( UILabel *label, const float value )
    {
        if( SigFigs == 0 )
        {
            label->SetText(std::to_string( value ).c_str() );
        }
		else if( SigFigs == -1 )
		{
			if( (1 * value) > (1 * Min))
				label->SetText( std::to_string( value ).c_str() );
			else
				label->SetText( "Default" );
		}
		else if( SigFigs < -1 )
		{ // Hex, just for fun
            label->SetText( std::to_string( value ).c_str());
        }
        else if( SigFigs > 1000)
        { // Why are you adjusting numbers large enough to need exponent notation with a slider?
            //label->SetText( StringUtils::Va( "%.*g", SigFigs - 1000, value ) );
			label->SetText( std::to_string( value ).c_str() );
        }
        else
        {
            //label->SetText( StringUtils::Va( "%.*f", SigFigs, value ) );
			label->SetText( std::to_string( value ).c_str() );
        }
    }

    eMsgStatus SliderComponent::OnEvent_Impl( OvrGuiSys & guiSys, ovrApplFrameIn const & vrFrame,
                                              VRMenuObject * self, VRMenuEvent const & event )
    {
        switch( event.EventType )
        {
            case VRMENU_EVENT_FOCUS_GAINED:
                HasFocus = true;
                return MSG_STATUS_ALIVE;

            case VRMENU_EVENT_FOCUS_LOST:
                HasFocus = false;
                return MSG_STATUS_ALIVE;

            case VRMENU_EVENT_TOUCH_DOWN:
                TouchDown = true;
                OnClick( guiSys, vrFrame, event );
                return MSG_STATUS_ALIVE;

            case VRMENU_EVENT_FRAME_UPDATE:
                return OnFrame( guiSys, vrFrame, self, event );

            default:
                OVR_ASSERT( !"Event flags mismatch!" );
                return MSG_STATUS_ALIVE;
        }
    }

    eMsgStatus SliderComponent::OnFrame( OvrGuiSys & guiSys, ovrApplFrameIn const & vrFrame,
                                         VRMenuObject * self, VRMenuEvent const & event )
    {
        if ( TouchDown )
        {/*
            if ( ( vrFrame.AllTouches & ( BUTTON_A | BUTTON_TOUCH ) ) != 0 )
            {
                OnClick( guiSys, vrFrame, event );
            }
            else*/
            {
                TouchDown = false;
            }
        }

        SeekTime->SetVisible( HasFocus );
        if ( HasFocus )
        {
            Vector3f hitPos = event.HitResult.RayStart + event.HitResult.RayDir * event.HitResult.t;

            // move hit position into local space
            const Posef modelPose = Background->GetWorldPose();

            Vector3f localHit = modelPose.Rotation.Inverted().Rotate( hitPos - modelPose.Translation );
            Bounds3f bounds = Background->GetMenuObject()->GetLocalBounds(guiSys.GetDefaultFont() ) * Background->GetParent()->GetWorldScale();
            const float progress = ( localHit.x - bounds.GetMins().x ) / bounds.GetSize().x;

            if ( ( progress >= 0.0f ) && ( progress <= 1.0f ) )
            {
                const float seekwidth = ScrubBarWidth * progress;
                Vector3f pos = SeekTime->GetLocalPosition();
                pos.x = PixelScale( ScrubBarWidth * -0.5f + seekwidth );
                SeekTime->SetLocalPosition( pos );

                SetText( SeekTime, ScaleValue(progress) );
            }
        }

        return MSG_STATUS_ALIVE;
    }

    void SliderComponent::OnClick( OvrGuiSys & guiSys, ovrApplFrameIn const & vrFrame, VRMenuEvent const & event )
    {
        if ( OnClickFunction == NULL )
        {
            return;
        }

        Vector3f hitPos = event.HitResult.RayStart + event.HitResult.RayDir * event.HitResult.t;

        // move hit position into local space
        const Posef modelPose = Background->GetWorldPose();
        Vector3f localHit = modelPose.Rotation.Inverted().Rotate( hitPos - modelPose.Translation );

        Bounds3f bounds = Background->GetMenuObject()->GetLocalBounds( guiSys.GetDefaultFont() ) * Background->GetParent()->GetWorldScale();
        const float progress = ( localHit.x - bounds.GetMins().x ) / bounds.GetSize().x;
        if ( ( progress >= 0.0f ) && ( progress <= 1.0f ) )
        {
            ( *OnClickFunction )( this, OnClickObject, ScaleValue(progress) );
        }
    }

} // namespace OculusCinema
