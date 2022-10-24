/************************************************************************************

Filename    :   UIButton.h
Content     :
Created     :	1/23/2015
Authors     :   Jim Dose

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#if !defined( UIButton_h )
#define UIButton_h

#include "VRMenu.h"
#include "UI/UIObject.h"
#include "VRMenuComponent.h"
#include "UI/UITexture.h"

namespace OVR {

class VrAppInterface;
class UIButton;

//==============================================================
// UIButtonComponent
class UIButtonComponent : public VRMenuComponent
{
public:
	static const int TYPE_ID = 159493;

					UIButtonComponent( UIButton & button );
	virtual			~UIButtonComponent();

	virtual int		GetTypeId() const { return TYPE_ID; }

	bool			IsPressed() const { return TouchDown; }

	void			SetTouchDownSnd( const char * touchDownSnd );
	void			SetTouchUpSnd( const char * touchUpSnd );

	void			OwnerDeleted() { Button = nullptr; }

	UIButton *		GetButton() const { return Button; }

private:
	UIButton *		Button;

    ovrSoundLimiter GazeOverSoundLimiter;
    ovrSoundLimiter DownSoundLimiter;
    ovrSoundLimiter UpSoundLimiter;

    bool			TouchDown;

	std::string		TouchDownSnd { "touch_down" };
	std::string		TouchUpSnd { "touch_up" };

private:
	// private assignment operator to prevent copying
	UIButtonComponent &	operator = ( UIButtonComponent & );

    virtual eMsgStatus      OnEvent_Impl( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame,
                                    VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus              FocusGained( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              FocusLost( OvrGuiSys & guiSys, ovrFrameInput const & vrFrame,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

//==============================================================
// UIButton

class UIButton : public UIObject
{
	friend class UIButtonComponent;

public:
	enum eButtonActionType
	{
		ClickOnUp,
		ClickOnDown
	};

										UIButton( OvrGuiSys &guiSys );
										~UIButton();

	void 								AddToMenu( UIMenu *menu, UIObject *parent = NULL );

	void 								SetButtonImages( const UITexture &normal, const UITexture &hover, const UITexture &pressed, const UIRectf &border = UIRectf() );
	void								SetButtonColors( const Vector4f &normal, const Vector4f &hover, const Vector4f &pressed );
	//Set this button up as a toggle button, have to provide an extra color and texture for when the button is down and being hovered over.
	void								SetAsToggleButton( const UITexture & pressedHoverTexture, const Vector4f & pressedHoverColor );
	//By default OnClick happens when button is released, but can be changed to happen on button down instead.
	void								SetActionType( const eButtonActionType actionType ) { ActionType = actionType; }
	void								SetText( const char * text );

	void								SetOnClick( void ( *callback )( UIButton *, void * ), void *object );
	void								SetOnFocusGained( void( *callback )( UIButton *, void * ), void *object );
	void								SetOnFocusLost( void( *callback )( UIButton *, void * ), void *object );

	void								UpdateButtonState();
	
	bool								IsPressed() const { return ButtonComponent->IsPressed(); }

	void								SetTouchDownSnd( const char * touchDownSnd );
	void								SetTouchUpSnd( const char * touchUpSnd );

private:
	UIButtonComponent *					ButtonComponent;
	
	const UITexture *					NormalTexture { nullptr };
	const UITexture *					HoverTexture { nullptr };
	const UITexture *					PressedTexture { nullptr };
	const UITexture *					PressedHoverTexture { nullptr }; //only used by toggle buttons

	Vector4f							NormalColor;
	Vector4f 							HoverColor;
	Vector4f 							PressedColor;
	Vector4f 							PressedHoverColor; //only used by toggle buttons

	bool								ToggleButton { false };
	eButtonActionType					ActionType { ClickOnUp };

	void 								( *OnClickFunction )( UIButton *button, void *object );
	void *								OnClickObject;

	void 								OnClick();

	void								( *OnFocusGainedFunction )( UIButton *button, void *object );
	void *								OnFocusGainedObject;

	void								FocusGained();

	void								( *OnFocusLostFunction )( UIButton *button, void *object );
	void *								OnFocusLostObject;

	void								FocusLost();

};

} // namespace OVR

#endif // UIButton_h
