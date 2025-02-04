/************************************************************************************

Filename    :   UIKeyboard.h
Content     :
Created     :	11/3/2015
Authors     :   Eric Duhon

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#if !defined( UIKeyboard_h )
#define UIKeyboard_h

#include "OVR_Math.h"
#include "UIMenu.h"

#include <string>

namespace OVR
{
	class OvrGuiSys;

	class UIKeyboard
	{
	public:
		enum KeyEventType
		{
			Text,
			Backspace,
			Return
		};
		using KeyPressEventT = void( *)( const KeyEventType, const std::string &, void * );

		UIKeyboard( const UIKeyboard & ) = delete;
		UIKeyboard & operator=( const UIKeyboard & ) = delete;

		//By default keyboard is centered around 0,0,0 with a size specified in the keyboard json file. Use positionOffset to shift the keyboard
		static UIKeyboard * Create( OvrGuiSys &	GuiSys, UIMenu * menu, const Vector3f & positionOffset = Vector3f::ZERO ); //Load the default keyboard set
		static UIKeyboard * Create( OvrGuiSys &	GuiSys, UIMenu * menu, const char * keyboardSet, const Vector3f & positionOffset = Vector3f::ZERO );
		static void Free( UIKeyboard * keyboard );

		virtual void SetKeyPressHandler( KeyPressEventT eventHandler, void * userData ) = 0;
		virtual bool OnKeyEvent( const int keyCode, const int repeatCount, const OVR::KeyEventType eventType ) = 0;
		
	protected:
		UIKeyboard( ) = default;
		virtual ~UIKeyboard( ) = default;
	};
}

#endif //UIKeyboard_h
