// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/Controller.h"

namespace AE::UI
{

	//
	// Gesture Recognizer
	//

	class GestureRecognizer
	{
	// types
	private:
		using Keys_t		= StaticArray< EKeyState, uint(EKey::_Count) >;
		using ETouchAction	= AE::App::InputEventQueue::TouchEvent::EAction;
		
		struct Touch
		{
			float2			startPos;	// dips
			float2			pos;
			float2			delta;		// dips
			float			pressure	= 0.0f;
			nanoseconds		time		{};		// time when button pressed
			nanoseconds		startTime	{};
			bool			motion	: 1;
			bool			pressed : 1;
			
			Touch () : motion{false}, pressed{false} {}
		};

		static constexpr uint	MaxTouches = 8;
		using Touches_t			= StaticArray< Touch, MaxTouches >;
		using ActiveTouches_t	= BitSet< MaxTouches >;


	// variables
	private:
		InputState			_inputState;
		float2				_lastMousePos;	// dips
		bool				_lmbPressed		= false;

		Touches_t			_touches;
		ActiveTouches_t		_activeTouches;
		uint				_firstTouchId	= UMax;

		struct {
			nanoseconds			lastTapTime	{0};
			uint				touchId		= 0;
			bool				isActive	= false;
		}					_tapRecognizer;

		struct {
			float2				prevPos;
			uint				touchId		= 0;
			bool				isActive	= false;
		}					_dragRecognizer;


	// methods
	public:
		GestureRecognizer ();
		
		void  BeforeUpdate ();
		void  AfterUpdate ();

		void  ProcessEvents (const SurfaceDimensions &, App::InputEventQueue &);

		ND_ InputState &	GetInputState ()	{ return _inputState; }


	private:
		void  _OnKey (EKey code, EKeyState state);
		void  _OnTouch (uint id, const float2 &pos, float pressure, ETouchAction act, nanoseconds time);

		void  _RecognizeTaps (uint activeCount, nanoseconds time);
		void  _RecognizeDragging (uint activeCount, nanoseconds time);
	};


}	// AE::UI
