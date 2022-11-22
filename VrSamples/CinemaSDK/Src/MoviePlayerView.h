/************************************************************************************

Filename    :   MoviePlayerView.h
Content     :
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( MoviePlayerView_h )
#define MoviePlayerView_h

#include "GUI/GazeCursor.h"
#include "GUI/GuiSys.h"
#include "GUI/Lerp.h"
#include "GUI/UI/UITexture.h"
#include "GUI/UI/UIMenu.h"
#include "GUI/UI/UIContainer.h"
#include "GUI/UI/UILabel.h"
#include "GUI/UI/UIImage.h"
#include "GUI/UI/UIButton.h"
#include "Settings.h"


namespace OculusCinema {

enum AspectRatio
{
	Ratio_16_9 = 0,
	Ratio_4_3,
};

enum MouseMode
{
	MOUSE_OFF = 0,
	MOUSE_GAZE,
	MOUSE_TRACKPAD
};

class CinemaApp;

class ControlsGazeTimer : public OVRFW::VRMenuComponent
{
public:
	static const int 		TYPE_ID = 152413;

							ControlsGazeTimer();

	virtual int				GetTypeId() const { return TYPE_ID; }

	void					SetGazeTime( const double currTimeInSeconds );
	double					GetLastGazeTime() const { return LastGazeTime; }

	bool					IsFocused() const { return HasFocus; }

private:
	double					LastGazeTime;
	bool					HasFocus;

private:
	virtual OVRFW::eMsgStatus		OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
												   OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );
};

class SliderComponent : public OVRFW::VRMenuComponent
{
public:
	static const int 		TYPE_ID = 152414;

							SliderComponent();

	virtual int				GetTypeId() const { return TYPE_ID; }

	void                    SetExtents( const float max, const float min, const int sigfigs );
	void                    SetOnClick( void ( *callback )( SliderComponent *, void *, float ), void *object );
	void					SetWidgets( OVRFW::UIObject *background, OVRFW::UIObject *scrubBar, OVRFW::UILabel *currentTime, OVRFW::UILabel *seekTime, const int scrubBarWidth );
	void 					SetProgress( const float progress );
	void                    SetValue( const float value );

	float                    ScaleValue(const float value);
	bool					IsScrubbing() const { return TouchDown; }

private:
	bool					HasFocus;
	bool					TouchDown;

	float					Progress;
	float                    Max;
	float                    Min;
	int                        SigFigs;

	OVRFW::UIObject *				Background;
	OVRFW::UIObject *				ScrubBar;
	OVRFW::UILabel *				CurrentTime;
	OVRFW::UILabel *				SeekTime;
	int 					ScrubBarWidth;
	void                     ( *OnClickFunction )( SliderComponent *button, void *object, float progress );

	void *					OnClickObject;

private:
	virtual OVRFW::eMsgStatus      OnEvent_Impl( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
										  OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

	OVRFW::eMsgStatus 				OnFrame( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame,
									   OVRFW::VRMenuObject * self, OVRFW::VRMenuEvent const & event );

	void 					OnClick( OVRFW::OvrGuiSys & guiSys, OVRFW::ovrApplFrameIn const & vrFrame, OVRFW::VRMenuEvent const & event );

	void                    SetText( OVRFW::UILabel *label, const float value );
};

class MoviePlayerView : public View
{
public:
							MoviePlayerView( CinemaApp & app_ );
	virtual 				~MoviePlayerView();

	virtual void 			OneTimeInit( const char * launchIntent );
	virtual void			OneTimeShutdown();

	virtual void 			OnOpen( const double currTimeInSeconds );
	virtual void 			OnClose();

	virtual void			EnteredVrMode();
	virtual void 			LeavingVrMode();

	virtual bool 			OnKeyEvent( const int keyCode, const int repeatCount, const OVRFW::UIKeyboard::KeyEventType eventType );
	virtual void 			Frame( const OVRFW::ovrApplFrameIn & vrFrame );

	virtual void            SetError( const char *text, bool showErrorIcon );

	void                    MovieScreenUpdated();


private:
	CinemaApp &				Cinema;

	bool					uiActive;

	bool					RepositionScreen;



	static const double 	GazeTimeTimeout;

	OVRFW::UITexture				BackgroundTintTexture;

	OVRFW::UITexture				SliderBackgroundTexture;
	OVRFW::UITexture				SliderProgressTexture;

	OVRFW::UITexture				SliderPosition;

	OVRFW::UIMenu *				MoveScreenMenu;
	OVRFW::UILabel *				MoveScreenLabel;
	OVRFW::Lerp					MoveScreenAlpha;

	OVRFW::UIMenu *				PlaybackControlsMenu;
	OVRFW::UIContainer *			PlaybackControlsPosition;
	OVRFW::UIContainer *			PlaybackControlsScale;
	OVRFW::UILabel *				MovieTitleLabel;


	OVRFW::UIImage	*				ControlsBackground;
	ControlsGazeTimer *		GazeTimer;

	OVRFW::UITexture                MouseTexture;
	OVRFW::UITexture                MouseHoverTexture;
	OVRFW::UITexture                MousePressedTexture;

	OVRFW::UITexture                StreamTexture;
	OVRFW::UITexture                StreamHoverTexture;
	OVRFW::UITexture                StreamPressedTexture;

	OVRFW::UITexture                ScreenTexture;
	OVRFW::UITexture                ScreenHoverTexture;
	OVRFW::UITexture                ScreenPressedTexture;

	OVRFW::UITexture                ExitTexture;
	OVRFW::UITexture                ExitHoverTexture;
	OVRFW::UITexture                ExitPressedTexture;

	OVRFW::UIContainer *			SaveMenu;
	OVRFW::UIButton	*		ButtonSaveApp;
	OVRFW::UIButton	*		ButtonSaveDefault;
	OVRFW::UIButton	*		ButtonResetSettings;
	OVRFW::UIButton	*		ButtonSaveSettings1;
	OVRFW::UIButton	*		ButtonSaveSettings2;
	OVRFW::UIButton	*		ButtonSaveSettings3;
	OVRFW::UIButton	*		ButtonLoadSettings1;
	OVRFW::UIButton	*		ButtonLoadSettings2;
	OVRFW::UIButton	*		ButtonLoadSettings3;

	OVRFW::UITexture                ButtonTexture;
	OVRFW::UITexture                ButtonHoverTexture;
	OVRFW::UITexture                ButtonPressedTexture;


	OVRFW::UIButton *               MouseMenuButton;
	OVRFW::UIContainer *           MouseMenu;
	OVRFW::UIButton *           	ButtonGaze;
	OVRFW::UIButton *           	ButtonTrackpad;
    OVRFW::UIButton *           	ButtonOff;
	OVRFW::UILabel *                GazeScale;
	OVRFW::UIImage *                GazeSliderBackground;
	OVRFW::UIImage *                GazeSliderIndicator;
	OVRFW::UILabel *                GazeCurrentSetting;
	OVRFW::UILabel *                GazeNewSetting;
	SliderComponent         GazeSlider;

	OVRFW::UILabel *                TrackpadScale;
	OVRFW::UIImage *                TrackpadSliderBackground;
	OVRFW::UIImage *                TrackpadSliderIndicator;
	OVRFW::UILabel *                TrackpadCurrentSetting;
	OVRFW::UILabel *                TrackpadNewSetting;
	SliderComponent         TrackpadSlider;


	OVRFW::UIButton *               StreamMenuButton;
	OVRFW::UIContainer *            StreamMenu;
	OVRFW::UIButton *           ButtonAspectRatio169;
	OVRFW::UIButton *           ButtonAspectRatio43;
	OVRFW::UIButton *           Button4k60;
	OVRFW::UIButton *           Button4k30;
    OVRFW::UIButton *           Button1080p60;
    OVRFW::UIButton *           Button1080p30;
    OVRFW::UIButton *           Button720p60;
    OVRFW::UIButton *           Button720p30;
	OVRFW::UIButton *           ButtonHostAudio;

	OVRFW::UIButton *           ButtonApply;

	OVRFW::UILabel *            BitrateAdjust;
	OVRFW::UIImage *            BitrateSliderBackground;
	OVRFW::UIImage *            BitrateSliderIndicator;
	OVRFW::UILabel *            BitrateCurrentSetting;
	OVRFW::UILabel *            BitrateNewSetting;
	SliderComponent     BitrateSlider;


	OVRFW::UIButton *               ScreenMenuButton;
	OVRFW::UIContainer *            ScreenMenu;
	OVRFW::UILabel *                  ScreenDistance;
	OVRFW::UIImage *                   DistanceSliderBackground;
	OVRFW::UIImage *                   DistanceSliderIndicator;
	OVRFW::UILabel *                DistanceCurrentSetting;
	OVRFW::UILabel *                DistanceNewSetting;
	SliderComponent         DistanceSlider;

	OVRFW::UILabel *                   ScreenSize;
	OVRFW::UIImage *                   SizeSliderBackground;
	OVRFW::UIImage *                   SizeSliderIndicator;
	OVRFW::UILabel *                SizeCurrentSetting;
	OVRFW::UILabel *                SizeNewSetting;
	SliderComponent         SizeSlider;
	OVRFW::UIButton *               ExitButton;

	float					settingsVersion;
	std::string					defaultSettingsPath;
	std::string					settings1Path;
	std::string					settings2Path;
	std::string					settings3Path;
	std::string					appSettingsPath;
	Settings*				defaultSettings;
	Settings*				settings1;
	Settings*				settings2;
	Settings*				settings3;
	Settings*				appSettings;


	bool					BackgroundClicked;
	bool					UIOpened;							// Used to ignore button A or touchpad until release so we don't close the UI immediately after opening it
	//float                   s00,s01,s10,s11,s20,s21;        // Last stick positions so we don't have to make network traffic when not changing
	bool                    allowDrag;
	bool					mouseMoving;
	double                  clickStartTime;
	signed char             lastScroll;
	OVR::Vector2f                lastMouse;
	bool                    mouseDownLeft;
	bool                    mouseDownRight;
	bool                    mouseDownMiddle;

	MouseMode				mouseMode;
	float                   gazeScaleValue;
	float                   trackpadScaleValue;

	AspectRatio				streamAspectRatio;
	int						streamWidth;
	int 					streamHeight;
	int						streamFPS;
	bool					streamHostAudio;
	float                   customBitrate;
	int                     bitrate;

    bool                    videoSettingsUpdated;
	float                   BitrateMin;
	float                   BitrateMax;

	float					GazeMin;
	float					GazeMax;
	float					TrackpadMin;
	float					TrackpadMax;
	float					VoidScreenDistanceMin;
	float					VoidScreenDistanceMax;
	float					VoidScreenScaleMin;
	float					VoidScreenScaleMax;
	class OldScreenPose : public std::list<OldScreenPose> {
		public: long time;
		OVR::Matrix4f pose;
	};
	std::list<OldScreenPose>        oldPoses;
	int                        calibrationStage;
	OVR::Matrix4f                lastPose;
	float                    trackCalibrationYaw;
	float                    trackCalibrationPitch;
	int                        latencyAddition;
	bool                    screenMotionPaused;


private:
	void					TextButtonHelper(OVRFW::UIButton* button, float scale = 1.0f, int w = 320, int h = 120);
	void                    SetUpSlider(OVRFW::OvrGuiSys & guiSys, OVRFW::UIObject *parent, SliderComponent& scrub, OVRFW::UIImage* bg,
										OVRFW::UIImage* ind, OVRFW::UILabel* cur, OVRFW::UILabel* set, int slideWidth, int xoff, int yoff);

	void 					CreateMenu( OVRFW::OvrGuiSys & guiSys );


	void					BackPressed();
	void					BackPressedDouble();

	
	friend void        MouseMenuButtonCallback( OVRFW::UIButton *button, void *object );
	void            MouseMenuButtonPressed();
	friend void        StreamMenuButtonCallback( OVRFW::UIButton *button, void *object );
	void            StreamMenuButtonPressed();
	friend void        ScreenMenuButtonCallback( OVRFW::UIButton *button, void *object );
	void            ScreenMenuButtonPressed();
	friend void        ExitButtonCallback( OVRFW::UIButton *button, void *object );
	void            ExitButtonPressed();


	friend void		SaveAppCallback( OVRFW::UIButton *button, void *object );
	void			SaveAppPressed();
	friend void		SaveDefaultCallback( OVRFW::UIButton *button, void *object );
	void			SaveDefaultPressed();
	friend void		ResetDefaultCallback( OVRFW::UIButton *button, void *object );
	void			ResetDefaultPressed();
	friend void		Save1Callback( OVRFW::UIButton *button, void *object );
	void			Save1Pressed();
	friend void		Save2Callback( OVRFW::UIButton *button, void *object );
	void			Save2Pressed();
	friend void		Save3Callback( OVRFW::UIButton *button, void *object );
	void			Save3Pressed();
	friend void		Load1Callback( OVRFW::UIButton *button, void *object );
	void			Load1Pressed();
	friend void		Load2Callback( OVRFW::UIButton *button, void *object );
	void			Load2Pressed();
	friend void		Load3Callback( OVRFW::UIButton *button, void *object );
	void			Load3Pressed();
	friend void        GazeCallback( OVRFW::UIButton *button, void *object );
	void            GazePressed();
	friend void        TrackpadCallback( OVRFW::UIButton *button, void *object );
	void            TrackpadPressed();
	friend void        OffCallback( OVRFW::UIButton *button, void *object );
	void            OffPressed();
	friend void        GazeScaleCallback( SliderComponent *button, void *object, const float value );
	void            GazeScalePressed(const float value);
	friend void        TrackpadScaleCallback( SliderComponent *button, void *object, const float value );
	void            TrackpadScalePressed(const float value);
	friend void     ButtonAspectRatio169Callback( OVRFW::UIButton *button, void *object );
	void            ButtonAspectRatio169Pressed();
	friend void     ButtonAspectRatio43Callback( OVRFW::UIButton *button, void *object );
	void            ButtonAspectRatio43Pressed();
	friend void        Button4k60Callback( OVRFW::UIButton *button, void *object );
	void            Button4k60Pressed();
	friend void        Button4k30Callback( OVRFW::UIButton *button, void *object );
	void            Button4k30Pressed();
	friend void        Button1080p60Callback( OVRFW::UIButton *button, void *object );
	void            Button1080p60Pressed();
	friend void        Button1080p30Callback( OVRFW::UIButton *button, void *object );
	void            Button1080p30Pressed();
	friend void        Button720p60Callback( OVRFW::UIButton *button, void *object );
	void            Button720p60Pressed();
	friend void        Button720p30Callback( OVRFW::UIButton *button, void *object );
	void            Button720p30Pressed();
	friend void        HostAudioCallback( OVRFW::UIButton *button, void *object );
	void            HostAudioPressed();
	friend void        ApplyVideoCallback( OVRFW::UIButton *button, void *object );
	void            ApplyVideoPressed();



	friend bool        ApplyVideoIsEnabledCallback( OVRFW::UIButton *button, void *object );
	bool            ApplyVideoIsEnabled();
	friend void        BitrateCallback( SliderComponent *button, void *object, const float value );
	void            BitratePressed( const float value);
	friend void        DistanceCallback( SliderComponent *button, void *object, const float value );
	void            DistancePressed( const float value);
	friend void        SizeCallback( SliderComponent *button, void *object, const float value );
	void            SizePressed( const float value);

    void                    UpdateMenus();


	OVR::Vector2f 				GazeCoordinatesOnScreen( const OVR::Matrix4f & viewMatrix, const OVR::Matrix4f panelMatrix ) const;

	void					LoadSettings(Settings* set);
	void					InitializeSettings();

	void 					UpdateUI( const OVRFW::ovrApplFrameIn & vrFrame );
	void 					CheckInput( const OVRFW::ovrApplFrameIn & vrFrame );
    void                    HandleGazeMouse( const OVRFW::ovrApplFrameIn & vrFrame, bool onscreen, const OVR::Vector2f screenCursor );
    void                    HandleTrackpadMouse( const OVRFW::ovrApplFrameIn & vrFrame );

    void 					CheckDebugControls( const OVRFW::ovrApplFrameIn & vrFrame );

	void 					ShowUI(const double currTimeInSeconds);
	void 					HideUI();

	OVR::Matrix4f                InterpolatePoseAtTime(long time);
	void                    RecordPose( long time, OVR::Matrix4f pose );
	void                    CheckVRInput( const OVRFW::ovrApplFrameIn & vrFrame );
	void                    HandleCalibration( const OVRFW::ovrApplFrameIn & vrFrame );

	int HeightAspectRatio43();

    int WidthAspectRatio43();

	int WidthByAspect();


};

} // namespace OculusCinema

#endif // MoviePlayerView_h
