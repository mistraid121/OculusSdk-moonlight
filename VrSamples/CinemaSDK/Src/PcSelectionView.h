/************************************************************************************

Filename    :   PcSelectionView.h
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( PcSelectionView_h )
#define PcSelectionView_h

#include <GUI/UI/UIButton.h>
#include "vector"
#include "GUI/Lerp.h"
#include "SelectionView.h"
#include "CarouselBrowserComponent.h"
#include "PcManager.h"
#include "GUI/UI/UITexture.h"
#include "GUI/UI/UIMenu.h"
#include "GUI/UI/UIContainer.h"
#include "GUI/UI/UILabel.h"
#include "GUI/UI/UIImage.h"


namespace OculusCinema {

class CinemaApp;


class PcSelectionView : public SelectionView
{
public:
						PcSelectionView( CinemaApp &cinema );
	virtual 			~PcSelectionView();

	virtual void 		OneTimeInit( const char * launchIntent );
	virtual void		OneTimeShutdown();

	virtual void 		OnOpen(const double currTimeInSeconds );
	virtual void 		OnClose();

	virtual bool 		OnKeyEvent( const int keyCode, const int repeatCount, const OVRFW::UIKeyboard::KeyEventType eventType );
	virtual void 		Frame( const OVRFW::ovrApplFrameIn & vrFrame );

	void 				SetPcList( const std::vector<const PcDef *> &pcs);

	virtual void 				Select( void );
	virtual void 				SelectionHighlighted( bool isHighlighted );
	virtual void 				SetCategory( const PcCategory category );
	virtual void				SetError( const char *text, bool showErrorIcon );
	virtual void				ClearError();

private:
	class PcCategoryButton
	{
	public:
		PcCategory 	Category;
		std::string			Text;
		OVRFW::UILabel *		Button;
		float			Width;
		float			Height;

		PcCategoryButton( const PcCategory category, const std::string &text ) :
						Category( category ), Text( text ), Button( NULL ), Width( 0.0f ), Height( 0.0f ) {}
	};

private:
	CinemaApp &							Cinema;

	OVRFW::UITexture 							SelectionTexture;
	OVRFW::UITexture							Is3DIconTexture;
	OVRFW::UITexture							ShadowTexture;
	OVRFW::UITexture							BorderTexture;
	OVRFW::UITexture							SwipeIconLeftTexture;
	OVRFW::UITexture							SwipeIconRightTexture;
	OVRFW::UITexture							ResumeIconTexture;
	OVRFW::UITexture							ErrorIconTexture;
	OVRFW::UITexture							CloseIconTexture;

	OVRFW::UIMenu *							Menu;

	OVRFW::UIContainer *						CenterRoot;

	OVRFW::UILabel * 							ErrorMessage;
	OVRFW::UILabel * 							PlainErrorMessage;
	
	bool								ErrorMessageClicked;

	OVRFW::UIContainer *						PcRoot;
	OVRFW::UIContainer *						CategoryRoot;
	OVRFW::UIContainer *						TitleRoot;

	OVRFW::UILabel	*							PcTitle;

	OVRFW::UIImage *							SelectionFrame;

	OVRFW::UIImage *							CenterPoster;
	OVR::UPInt								CenterIndex;
	OVR::Vector3f							CenterPosition;

	OVRFW::UIImage *							LeftSwipes[ 3 ];
	OVRFW::UIImage * 							RightSwipes[ 3 ];

	OVRFW::UILabel	*							ResumeIcon;
	OVRFW::UIButton *                            CloseAppButton;

	OVRFW::UILabel *							MoveScreenLabel;
	OVRFW::Lerp								MoveScreenAlpha;

	OVRFW::Lerp								SelectionFader;

	CarouselBrowserComponent *			PcBrowser;
	std::vector<CarouselItem *> 				PcBrowserItems;
	std::vector<PanelPose>					PcPanelPositions;

	std::vector<CarouselItemComponent *>	 	PcPosterComponents;

	std::vector<PcCategoryButton>			Categories;
	PcCategory			 			CurrentCategory;
	
	std::vector<const PcDef *> 			PcList;
	int									PcsIndex;

	const PcDef *					LastPcDisplayed;

	bool								RepositionScreen;
	bool								HadSelection;
	int                                    newPCWidth;
	int                                    newPCHeight;
	GLuint                                newPCTex;

    OVRFW::UITexture                ButtonTexture;
    OVRFW::UITexture                ButtonHoverTexture;
    OVRFW::UITexture                ButtonPressedTexture;

	OVRFW::UIContainer *                        newPCMenu;
	OVRFW::UITexture                            bgTintTexture;
	OVRFW::UIImage *                               newPCbg;
	OVRFW::UILabel *                               newPCIPLabel;
	std::vector<OVRFW::UIButton*>                newPCIPButtons;
	int                                 IPoctets[4];
	int                                    currentOctet;
	std::string                                IPString;



private:
	PcSelectionView &				operator=( const PcSelectionView & );

	const PcDef *					GetSelectedPc() const;

	void 								CreateMenu( OVRFW::OvrGuiSys & guiSys );
	OVR::Vector3f 							ScalePosition( const OVR::Vector3f &startPos, const float scale, const float menuOffset ) const;
	void 								UpdateMenuPosition();

	void								UpdatePcTitle();
	void								UpdateSelectionFrame( const OVRFW::ovrApplFrameIn & vrFrame );

	bool								ErrorShown() const;
	friend void                         NewPCIPButtonCallback( OVRFW::UIButton *button, void *object );
	void                                NewPCIPButtonPressed( OVRFW::UIButton *button );

	friend void                         CloseAppButtonCallback( OVRFW::UIButton *button, void *object );
	void                                CloseAppButtonPressed();


	bool                                BackPressed();

};

} // namespace OculusCinema

#endif // PcSelectionView_h
