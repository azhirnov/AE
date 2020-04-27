// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/Common.h"

namespace AE::App
{

	enum class EKeyState : uint8_t
	{
		Down,		// just pressed
		Hold,		// held down
		Up,			// released
		Unknown,
	};


	enum class EKey : uint16_t
	{
		Unknown,

		// mouse buttons
		MB_1,
		MB_Left		= MB_1,
		MB_2,
		MB_RightMB	= MB_2,
		MB_3,
		MB_Middle	= MB_3,
		MB_4,
		MB_5,
		MB_6,
		MB_7,
		MB_8,

		// mouse wheel
		MouseWheelUp,
		MouseWheelDown,
		MouseWheelLeft,
		MouseWheelRight,

		// keys
		n0,
		n1,
		n2,
		n3,
		n4,
		n5,
		n6,
		n7,
		n8,
		n9,

		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		
		ArrowLeft,
		ArrowRight,
		ArrowUp,
		ArrowDown,

		Space,
		Apostrophe,
		Comma,
		Minus,
		Period,
		Slash,
		Semicolon,
		Equal,
		LeftBracket,
		RightBracket,
		Backslash,
		Grave,
		Escape,
		Enter,
		Tab,
		Backspace,
		Insert,
		Delete,
		PageUp,
		PageDown,
		Home,
		End,
		CapsLock,
		ScrollLock,
		NumLock,
		PrintScreen,
		Pause,

		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,

		_Count
	};

}	// AE::App
