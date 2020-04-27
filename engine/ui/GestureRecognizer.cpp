// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/GestureRecognizer.h"

namespace AE::UI
{

/*
=================================================
	SetCursorState
=================================================
*/
	inline void  InputState::SetCursorState (const float2 &pos, bool pressed)
	{
		_cursor._position = pos;
		_cursor._pressed  = pressed;
	}
	
/*
=================================================
	BeforeUpdate
=================================================
*/
	inline void  InputState::BeforeUpdate ()
	{
		if ( _cursor._resetFocus )
			_cursor._focused = null;

		_cursor._resetFocus = false;
	}
	
/*
=================================================
	AddGesture / ClearGestures
=================================================
*/
	inline void  InputState::AddGesture (const GestureData &data)
	{
		_cursor._gestures.push_back( data );
	}
	
	inline void  InputState::ClearGestures ()
	{
		_cursor._gestures.clear();
	}
//-----------------------------------------------------------------------------
	
			
	inline InputState::GestureData::GestureData (const GestureData &other) :
		type{ other.type }
	{
		switch ( other.type )
		{
			case EGesture::Unknown :
				break;

			case EGesture::SingleTap :
				singleTap = other.singleTap;
				break;

			case EGesture::DoubleTap :
				doubleTap = other.doubleTap;
				break;

			case EGesture::LongPress :
				longPress = other.longPress;
				break;
			
			case EGesture::Rotation_Start :
			case EGesture::Rotation_Update :
			case EGesture::Rotation_Stop :
				rotation = other.rotation;
				break;

			case EGesture::Dragging_Start :
			case EGesture::Dragging_Update :
			case EGesture::Dragging_Stop :
				dragging = other.dragging;
				break;
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	GestureRecognizer::GestureRecognizer ()
	{
		STATIC_ASSERT( MaxTouches >= App::InputEventQueue::TouchEvent::MaxTouches );
	}
	
/*
=================================================
	BeforeUpdate
=================================================
*/
	void  GestureRecognizer::BeforeUpdate ()
	{
		_inputState.BeforeUpdate();
		
		const uint		active_count = uint(_activeTouches.count());
		Nanoseconds		time		 = App::Clock().Timestamp();

		_RecognizeTaps( active_count, time );
		_RecognizeDragging( active_count, time );
	}
	
/*
=================================================
	AfterUpdate
=================================================
*/
	void  GestureRecognizer::AfterUpdate ()
	{
		_inputState.ClearGestures();
	}

/*
=================================================
	ProcessEvents
=================================================
*/
	void  GestureRecognizer::ProcessEvents (const SurfaceDimensions &surfaceDim, App::InputEventQueue &eventQueue)
	{
		using EventType		= App::InputEventQueue::EventType;
		using KeyEvent		= App::InputEventQueue::KeyEvent;
		using TouchEvent	= App::InputEventQueue::TouchEvent;
		using MouseEvent	= App::InputEventQueue::MouseEvent;

		App::InputEventQueue::MsgStorage	msg;
		for (; eventQueue.Pop( OUT msg );)
		{
			switch ( msg.content.eventType )
			{
				case EventType::Key :
				{
					auto&	ev = *Cast<KeyEvent>( &msg.content );

					_OnKey( ev.key, ev.state );
					break;
				}

				case EventType::Mouse :
				{
					auto&	ev  = *Cast<MouseEvent>( &msg.content );
					float2	pos = surfaceDim.PixelsToDips({ float(ev.pos.x), float(ev.pos.y) });

					if ( _lmbPressed )
					{
						_OnTouch( 0, pos, 1.0f, ETouchAction::Move, ev.timestamp );
						_lastMousePos = pos;
					}
					else
						_inputState.SetCursorState( pos, false );

					break;
				}

				case EventType::Touch :
				{
					auto&	ev  = *Cast<TouchEvent>( &msg.content );

					for (uint i = 0, cnt = ev.count; i < cnt; ++i)
					{
						_OnTouch( uint(ev.data[i].id),
								  surfaceDim.UNormPixelsToDips( ev.data[i].pos ),
								  ev.data[i].pressure, ev.action, ev.timestamp );
					}
					break;
				}
			}
		}
	}
	
/*
=================================================
	_OnKey
=================================================
*/
	void  GestureRecognizer::_OnKey (EKey code, EKeyState state)
	{
		Unused( code, state );
	}

/*
=================================================
	_OnTouch
=================================================
*/
	void  GestureRecognizer::_OnTouch (const uint id, const float2 &pos, const float pressure, const ETouchAction act, const Nanoseconds time)
	{
		const bool	moved	= (act == ETouchAction::Move);
		const bool	pressed = (act == ETouchAction::Down or moved);

		_activeTouches.set( id, pressed );
		const uint	active_count = uint(_activeTouches.count());

		auto&	touch = _touches[ id ];

		// on new touch
		if ( (not touch.pressed) and pressed )
		{
			touch.startPos	= pos;
			touch.startTime	= time;

			// start tap recognizer
			if ( (not _tapRecognizer.isActive) and (active_count == 1) )
			{
				_tapRecognizer.touchId   = id;
				_tapRecognizer.isActive	 = true;
			}
		}

		const float2	delta = (moved ? pos - touch.pos : float2());

		touch.motion	= pressed and moved;
		touch.pressed	= pressed;
		touch.pressure	= pressure;
		touch.pos		= pos;
		touch.delta		+= delta;
		touch.time		= time;

		if ( active_count == 1 )
			_firstTouchId = pressed ? id : IntLog2( _activeTouches.to_ulong() );

		if ( _firstTouchId == id )
			_inputState.SetCursorState( pos, pressed );

		if ( active_count == 0 )
			_firstTouchId = UMax;
	}
	
/*
=================================================
	_RecognizeTaps
=================================================
*/
	void  GestureRecognizer::_RecognizeTaps (const uint activeCount, const Nanoseconds time)
	{
		static constexpr float			MaxDistanceSqr		= 4.0f;				// dips^2
		static constexpr Nanoseconds	MaxPressTime		{ 2'000'000'000 };	// 2 sec
		static constexpr Nanoseconds	DoubleTapTimeDelta	{ 500'000'000 };	// 0.5 sec

		if ( not _tapRecognizer.isActive )
			return;
		
		Touch const&	touch	= _touches[ _tapRecognizer.touchId ];
		Nanoseconds		dt		= (time - touch.startTime);
		float			dist	= DistanceSqr( touch.pos, touch.startPos );
			
		// cancel on multitouch
		if ( activeCount > 1 )
		{
			_tapRecognizer.isActive = false;
		}
		else
		// cancel if difference is tee big
		if ( dist > MaxDistanceSqr )
		{
			_tapRecognizer.isActive = false;
		}
		else
		// cancel if pressed for a long time
		if ( dt > MaxPressTime )
		{
			_tapRecognizer.isActive = false;
				
			// trigger long press event
			if ( touch.pressed )
			{
				InputState::GestureData		longpress_gesture;
				longpress_gesture.type			= InputState::EGesture::LongPress;
				longpress_gesture.longPress.pos	= touch.startPos;
				_inputState.AddGesture( longpress_gesture );
			}
		}
		else
		// stop
		if ( not touch.pressed )
		{
			dt = (time - _tapRecognizer.lastTapTime);

			_tapRecognizer.isActive		= false;
			_tapRecognizer.lastTapTime	= time;

			// single tap
			if ( dt > DoubleTapTimeDelta )
			{
				InputState::GestureData		tap_gesture;
				tap_gesture.type			= InputState::EGesture::SingleTap;
				tap_gesture.singleTap.pos	= touch.startPos;
				_inputState.AddGesture( tap_gesture );
			}
			else
			// double tap
			{
				_tapRecognizer.lastTapTime	= Nanoseconds{0};	// to forbid triple tap

				InputState::GestureData		tap_gesture;
				tap_gesture.type			= InputState::EGesture::DoubleTap;
				tap_gesture.doubleTap.pos	= touch.startPos;
				_inputState.AddGesture( tap_gesture );
			}
		}
	}
	
/*
=================================================
	_RecognizeDragging
=================================================
*/
	void  GestureRecognizer::_RecognizeDragging (uint activeCount, Nanoseconds)
	{
		static constexpr float	MinDistanceSqr	= 1.0f;		// dips^2

		// start
		if ( (not _dragRecognizer.isActive) and (not _tapRecognizer.isActive) and (activeCount == 1) )
		{
			Touch const&	touch = _touches[ _firstTouchId ];

			if ( touch.motion )
			{
				_dragRecognizer.prevPos		= touch.pos;
				_dragRecognizer.touchId		= _firstTouchId;
				_dragRecognizer.isActive	= true;

				InputState::GestureData		drag_gesture;
				drag_gesture.type			= InputState::EGesture::Dragging_Start;
				drag_gesture.dragging.pos	= touch.pos;
				drag_gesture.dragging.prev	= touch.startPos;
				_inputState.AddGesture( drag_gesture );
			}
			return;
		}

		if ( not _dragRecognizer.isActive )
			return;

		Touch const&	touch	= _touches[ _dragRecognizer.touchId ];
		float			dist	= DistanceSqr( touch.pos, _dragRecognizer.prevPos );
		
		// cancel on multitouch
		if ( activeCount > 1 )
		{
			_dragRecognizer.isActive = false;

			InputState::GestureData		drag_gesture;
			drag_gesture.type			= InputState::EGesture::Dragging_Stop;
			drag_gesture.dragging.pos	= touch.pos;
			drag_gesture.dragging.prev	= _dragRecognizer.prevPos;
			_inputState.AddGesture( drag_gesture );
		}
		else
		// stop
		if ( not touch.pressed )
		{
			_dragRecognizer.isActive = false;

			InputState::GestureData		drag_gesture;
			drag_gesture.type			= InputState::EGesture::Dragging_Stop;
			drag_gesture.dragging.pos	= touch.pos;
			drag_gesture.dragging.prev	= _dragRecognizer.prevPos;
			_inputState.AddGesture( drag_gesture );
		}
		else
		// update
		if ( touch.pressed and dist >= MinDistanceSqr )
		{
			InputState::GestureData		drag_gesture;
			drag_gesture.type			= InputState::EGesture::Dragging_Update;
			drag_gesture.dragging.pos	= touch.pos;
			drag_gesture.dragging.prev	= _dragRecognizer.prevPos;
			_inputState.AddGesture( drag_gesture );

			_dragRecognizer.prevPos = touch.pos;
		}
	}


}	// AE::UI
