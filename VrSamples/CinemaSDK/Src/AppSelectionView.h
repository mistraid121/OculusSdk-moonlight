/************************************************************************************

Filename    :   AppSelectionView.h
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( AppSelectionView_h )
#define AppSelectionView_h

#include "vector"
#include "GUI/Lerp.h"
#include "SelectionView.h"
#include "CarouselBrowserComponent.h"
#include "AppManager.h"
#include "GUI/UI/UITexture.h"
#include "GUI/UI/UIMenu.h"
#include "GUI/UI/UIContainer.h"
#include "GUI/UI/UILabel.h"
#include "GUI/UI/UIImage.h"
#include "GUI/UI/UIButton.h"
#include "Settings.h"



namespace OculusCinema {

class CinemaApp;

class AppSelectionView : public SelectionView
{
public:
						AppSelectionView( CinemaApp &cinema );
	virtual 			~AppSelectionView();

	virtual void 		OneTimeInit( const char * launchIntent );
	virtual void		OneTimeShutdown();

	virtual void 		OnOpen(const double currTimeInSeconds );
	virtual void 		OnClose();

	virtual void 		Frame( const OVRFW::ovrApplFrameIn & vrFrame );

	void                SetAppList( const std::vector<AppDef *> &apps);
    void                PairSuccess();

	void 				Select( void );
	void 				SelectionHighlighted( bool isHighlighted );
	void				SetError( const char *text, bool showErrorIcon );
	void				ClearError();

private:
	CinemaApp &							Cinema;

	OVRFW::UITexture 							SelectionTexture;
	OVRFW::UITexture							ShadowTexture;
	OVRFW::UITexture							SwipeIconLeftTexture;
	OVRFW::UITexture							SwipeIconRightTexture;
	OVRFW::UITexture							ResumeIconTexture;
	OVRFW::UITexture							ErrorIconTexture;
	OVRFW::UITexture                           CloseIconTexture;
	OVRFW::UITexture                           SettingsIconTexture;

	OVRFW::UITexture                			ButtonTexture;
	OVRFW::UITexture                			ButtonHoverTexture;
	OVRFW::UITexture                			ButtonPressedTexture;

	OVRFW::UIMenu *							Menu;

	OVRFW::UIContainer *						CenterRoot;

	OVRFW::UILabel * 							ErrorMessage;
	OVRFW::UILabel * 							PlainErrorMessage;
	
	bool								ErrorMessageClicked;

	OVRFW::UIContainer *						AppRoot;
	OVRFW::UIContainer *						TitleRoot;

	OVRFW::UILabel	*							AppTitle;

	OVRFW::UIImage *							SelectionFrame;

	OVRFW::UIImage *							CenterPoster;
	OVR::UPInt								CenterIndex;
	OVR::Vector3f							CenterPosition;

	OVRFW::UIImage *							LeftSwipes[ 3 ];
	OVRFW::UIImage * 							RightSwipes[ 3 ];

	OVRFW::UILabel	*							ResumeIcon;
	OVRFW::UIButton *                          CloseAppButton;
	OVRFW::UIButton *                          SettingsButton;

	OVRFW::UILabel *							MoveScreenLabel;
	OVRFW::Lerp								MoveScreenAlpha;

	OVRFW::Lerp								SelectionFader;

	CarouselBrowserComponent *			AppBrowser;
	std::vector<CarouselItem *> 				AppBrowserItems;
	std::vector<PanelPose>					AppPanelPositions;

	std::vector<CarouselItemComponent *>	 	AppPosterComponents;

	std::vector<AppDef *> 				AppList;
	int									AppIndex;

	const AppDef *					LastAppDisplayed;

	bool								RepositionScreen;
	bool								HadSelection;
	OVRFW::UIContainer *                        settingsMenu;
	OVRFW::UITexture                            bgTintTexture;
	OVRFW::UIImage *                               newPCbg;

	OVRFW::UIButton *                        ButtonGaze;
	OVRFW::UIButton *                        ButtonTrackpad;
	OVRFW::UIButton *                        ButtonOff;
	OVRFW::UIButton *						  Button169;
	OVRFW::UIButton *						  Button43;
	OVRFW::UIButton *                        Button4k60;
	OVRFW::UIButton *                        Button4k30;
	OVRFW::UIButton *                        Button1080p60;
	OVRFW::UIButton *                        Button1080p30;
	OVRFW::UIButton *                        Button720p60;
	OVRFW::UIButton *                        Button720p30;
	OVRFW::UIButton *                        ButtonHostAudio;
	OVRFW::UIButton *                        ButtonSaveApp;
	OVRFW::UIButton *                        ButtonSaveDefault;

	int                                    mouseMode;
	int										streamAspectRatio;
	int                                    streamWidth;
	int                                    streamHeight;
	int                                    streamFPS;
	bool                                streamHostAudio;

	float                                settingsVersion;
	std::string                                defaultSettingsPath;
	std::string                                appSettingsPath;
	Settings*                            defaultSettings;
	Settings*                            appSettings;



private:
	AppSelectionView &				operator=( const AppSelectionView & );
	void                            TextButtonHelper(OVRFW::UIButton* button, float scale = 1.0f, int w = 300, int h = 120);

	const AppDef *					GetSelectedApp() const;

	void 								CreateMenu( OVRFW::OvrGuiSys & guiSys );
	OVR::Vector3f 							ScalePosition( const OVR::Vector3f &startPos, const float scale, const float menuOffset ) const;
	void 								UpdateMenuPosition();

	void								UpdateAppTitle();
	void								UpdateSelectionFrame( const OVRFW::ovrApplFrameIn & vrFrame );

	friend void                            AppCloseAppButtonCallback( OVRFW::UIButton *button, void *object );
	void                                CloseAppButtonPressed();

	friend void                            SettingsButtonCallback( OVRFW::UIButton *button, void *object );
	void                                SettingsButtonPressed();

	friend void                            SettingsCallback( OVRFW::UIButton *button, void *object );
	void                                SettingsPressed( OVRFW::UIButton *button );

	friend bool                            SettingsActiveCallback( OVRFW::UIButton *button, void *object );
	bool                                SettingsIsActive( OVRFW::UIButton *button );

	bool                                BackPressed();

	bool								ErrorShown() const;
};

} // namespace OculusCinema

#endif // AppSelectionView_h
