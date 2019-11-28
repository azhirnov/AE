// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/App/Android/AndroidWindow.h"

#ifdef PLATFORM_ANDROID

# include "platform/App/Android/AndroidApplication.h"
# include <android/native_window_jni.h>
# include <android/keycodes.h>

namespace AE::App
{
namespace {
	EKey  _MapKey (jint keyCode);
	EKeyState  _MapKeyAction (jint action);
	InputEventQueueWriter::TouchEvent::EAction  _MapTouchAction (jint action);
}
	
/*
=================================================
	_GetAppWindow
=================================================
*/
	SharedPtr<AndroidWindow>  AndroidApplication::_GetAppWindow (WinID id)
	{
		auto*	app = _GetAppInstance();
		ASSERT( app );

		for (auto& obj_wnd : app->_windows)
		{
			if ( obj_wnd.first == id )
				return obj_wnd.second;
		}
		return null;
	}
/*
=================================================
	_GetNewWindow
=================================================
*/
	Pair< SharedPtr<AndroidWindow>, AndroidApplication::WinID >  AndroidApplication::_GetNewWindow ()
	{
		auto*	app = _GetAppInstance();
		ASSERT( app );

		auto	wnd = MakeShared<AndroidWindow>();
		return { wnd, app->_AddWindow( wnd )};
	}

/*
=================================================
	GetAppWindow
=================================================
*/
namespace {
	template <typename T>
	ND_ SharedPtr<AndroidWindow>  GetAppWindow (T uid)
	{
		return AndroidApplication::_GetAppWindow( uid );
	}
}
/*
=================================================
	constructor
=================================================
*/
	AndroidWindow::AndroidWindow ()
	{}
	
/*
=================================================
	destructor
=================================================
*/
	AndroidWindow::~AndroidWindow ()
	{
		EXLOCK( _drCheck );
		CHECK( _wndState == EState::Destroyed );
	}
	
/*
=================================================
	Close
=================================================
*/
	void  AndroidWindow::Close ()
	{
		EXLOCK( _drCheck );

		if ( _methods.close )
			_methods.close();
	}
	
/*
=================================================
	GetSurfaceSize
=================================================
*/
	uint2  AndroidWindow::GetSurfaceSize ()
	{
		EXLOCK( _drCheck );
		return _surfSize;
	}

/*
=================================================
	GetInputEventQueue
=================================================
*/
	InputEventQueue const&  AndroidWindow::GetInputEventQueue ()
	{
		EXLOCK( _drCheck );
		return _eventQueue;
	}
	
/*
=================================================
	GetNative
=================================================
*/
	NativeWindow  AndroidWindow::GetNative ()
	{
		EXLOCK( _drCheck );

		NativeWindow	result;
		result.nativeWindow	= _java.nativeWindow;
		return result;
	}
	
/*
=================================================
	_SetState
=================================================
*/
	void  AndroidWindow::_SetState (EState newState)
	{
		if ( Abs( int(_wndState) - int(newState) ) != 1 )
			return;

		_wndState = newState;

		if ( _listener )
		{
			_SetState2( _wndState );
		}
	}
	
	void  AndroidWindow::_SetState2 (EState newState)
	{
		BEGIN_ENUM_CHECKS();
		switch ( newState )
		{
			case EState::Created :		_listener->OnCreate( *this );			break;
			case EState::Started :		_listener->OnStart( *this );			break;
			case EState::InForeground :	_listener->OnEnterForeground( *this );	break;
			case EState::InBackground :	_listener->OnEnterBackground( *this );	break;
			case EState::Stopped :		_listener->OnStop( *this );				break;
			case EState::Destroyed :	_listener->OnDestroy( *this );			break;
			case EState::Unknown :
			default :					ASSERT( !"unknown state" );				break;
		}
		END_ENUM_CHECKS();
	}

/*
=================================================
	_SetListener
=================================================
*/
	void  AndroidWindow::_SetListener (UniquePtr<IWndListener> listener)
	{
		CHECK( not _listener );

		_listener = std::move(listener);
		
		if ( _wndState == EState::Destroyed )
		{
			_SetState2( EState::Created );
			_SetState2( EState::Destroyed );
			return;
		}

		if ( _wndState <= EState::InForeground )
		{
			for (EState s = EState::Created; s <= EState::InForeground; s = EState(uint(s) + 1))
			{
				_SetState2( s );
			}
		}
		else
		if ( _wndState == EState::InBackground )
		{
			_SetState2( EState::Created );
			_SetState2( EState::Started );
			_SetState2( EState::InBackground );
		}
		else
		if ( _wndState == EState::Stopped )
		{
			_SetState2( EState::Created );
			_SetState2( EState::Started );
			_SetState2( EState::Stopped );
		}
		
		if ( _java.nativeWindow )
			_listener->OnSurfaceCreated( *this );
	}

/*
=================================================
	native_OnCreate
=================================================
*/
	AndroidWindow::WinID JNICALL  AndroidWindow::native_OnCreate (JNIEnv *env, jclass, jobject jwnd)
	{
		auto[window, id] = AndroidApplication::_GetNewWindow();
		
		EXLOCK( window->_drCheck );

		JavaEnv	je{ env };

		window->_java.window	= JavaObj{ jwnd, je };

		window->_java.window.Method( "Close", window->_methods.close );

		window->_SetState( EState::Created );
		return id;
	}
	
/*
=================================================
	native_OnDestroy
=================================================
*/
	void JNICALL  AndroidWindow::native_OnDestroy (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			window->_SetState( EState::Destroyed );

			window->_java.window		= Default;
			window->_java.nativeWindow	= null;

			window->_methods.close		= Default;
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnStart
=================================================
*/
	void JNICALL  AndroidWindow::native_OnStart (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			window->_SetState( EState::Started );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnStop
=================================================
*/
	void JNICALL  AndroidWindow::native_OnStop (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			window->_SetState( EState::Stopped );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnEnterForeground
=================================================
*/
	void JNICALL  AndroidWindow::native_OnEnterForeground (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			window->_SetState( EState::InForeground );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnEnterBackground
=================================================
*/
	void JNICALL  AndroidWindow::native_OnEnterBackground (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			window->_SetState( EState::InBackground );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_SurfaceChanged
=================================================
*/
	void JNICALL  AndroidWindow::native_SurfaceChanged (JNIEnv *env, jclass, WinID wndId, jobject surface)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			const bool	is_created = not window->_java.nativeWindow;

			// on surface created
			if ( is_created )
			{
				window->_java.nativeWindow = ANativeWindow_fromSurface( env, surface );
			}
			CHECK_ERR( window->_java.nativeWindow, void());


			const uint2	old_size = window->_surfSize;

			window->_surfSize.x = uint(ANativeWindow_getWidth( window->_java.nativeWindow ));
			window->_surfSize.y = uint(ANativeWindow_getHeight( window->_java.nativeWindow ));

			if ( is_created and window->_listener )
			{
				window->_listener->OnSurfaceCreated( *window );
			}
			else
			if ( Any( old_size != window->_surfSize ) and window->_listener )
			{
				window->_listener->OnResize( *window, window->_surfSize );
			}
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_SurfaceDestroyed
=================================================
*/
	void JNICALL  AndroidWindow::native_SurfaceDestroyed (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			CHECK_ERR( window->_java.nativeWindow, void());

			if ( window->_listener )
			{
				window->_listener->OnSurfaceDestroyed( *window );
			}

			ANativeWindow_release( window->_java.nativeWindow );
			window->_java.nativeWindow = null;
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_Update
=================================================
*/
	void JNICALL  AndroidWindow::native_Update (JNIEnv*, jclass, WinID wndId)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			EXLOCK( window->_drCheck );

			AndroidApplication::_GetAppInstance()->Update();

			if ( window->_listener )
				window->_listener->OnUpdate( *window );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnKey
=================================================
*/
	void JNICALL  AndroidWindow::native_OnKey (JNIEnv*, jclass, WinID wndId, jint keycode, jint action)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			InputEventQueue::KeyEvent	ev;
			ev.key		= _MapKey( keycode );
			ev.state	= _MapKeyAction( action );

			window->_eventQueue.Push( ev );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnTouch
=================================================
*/
	void JNICALL  AndroidWindow::native_OnTouch (JNIEnv *env, jclass, WinID wndId, jint action, jint count, jfloatArray touchData)
	{
		if ( auto window = GetAppWindow( wndId ))
		{
			JavaArray<jfloat>	touches{ touchData, /*readOnly*/true, JavaEnv{env} };

			InputEventQueue::TouchEvent	ev;
			ev.count	= uint8_t(count);
			ev.action	= _MapTouchAction( action );

			for (int i = 0, j = 0; i < count; ++i)
			{
				ev.data[i].pos.x	= touches[j++];
				ev.data[i].pos.y	= touches[j++];
				ev.data[i].pressure	= touches[j++];
				ev.data[i].id		= uint8_t(touches[j++]);
			}

			window->_eventQueue.Push( ev );
		}
		else
		{
			ASSERT( !"unknown window" );
		}
	}
	
/*
=================================================
	native_OnOrientationChanged
=================================================
*/
	void JNICALL  AndroidWindow::native_OnOrientationChanged (JNIEnv*, jclass, WinID wndId, jint newOrientation)
	{
	}

/*
=================================================
	_MapKey
=================================================
*/
namespace {
	EKey  _MapKey (jint keyCode)
	{
		switch ( keyCode )
		{
			case AKEYCODE_0 :			return EKey::n0;
			case AKEYCODE_1 :			return EKey::n1;
			case AKEYCODE_2 :			return EKey::n2;
			case AKEYCODE_3 :			return EKey::n3;
			case AKEYCODE_4 :			return EKey::n4;
			case AKEYCODE_5 :			return EKey::n5;
			case AKEYCODE_6 :			return EKey::n6;
			case AKEYCODE_7 :			return EKey::n7;
			case AKEYCODE_8 :			return EKey::n8;
			case AKEYCODE_9 :			return EKey::n9;
			
			case AKEYCODE_A :			return EKey::A;
			case AKEYCODE_B :			return EKey::B;
			case AKEYCODE_C :			return EKey::C;
			case AKEYCODE_D :			return EKey::D;
			case AKEYCODE_E :			return EKey::E;
			case AKEYCODE_F :			return EKey::F;
			case AKEYCODE_G :			return EKey::G;
			case AKEYCODE_H :			return EKey::H;
			case AKEYCODE_I :			return EKey::I;
			case AKEYCODE_J :			return EKey::J;
			case AKEYCODE_K :			return EKey::K;
			case AKEYCODE_L :			return EKey::L;
			case AKEYCODE_M :			return EKey::M;
			case AKEYCODE_N :			return EKey::N;
			case AKEYCODE_O :			return EKey::O;
			case AKEYCODE_P :			return EKey::P;
			case AKEYCODE_Q :			return EKey::Q;
			case AKEYCODE_R :			return EKey::R;
			case AKEYCODE_S :			return EKey::S;
			case AKEYCODE_T :			return EKey::T;
			case AKEYCODE_U :			return EKey::U;
			case AKEYCODE_V :			return EKey::V;
			case AKEYCODE_W :			return EKey::W;
			case AKEYCODE_X :			return EKey::X;
			case AKEYCODE_Y :			return EKey::Y;
			case AKEYCODE_Z :			return EKey::Z;
				
			case AKEYCODE_MINUS :		return EKey::Minus;
			case AKEYCODE_EQUALS :		return EKey::Equal;
			case AKEYCODE_GRAVE :		return EKey::Grave;
			case AKEYCODE_BACKSLASH :	return EKey::Backslash;
			case AKEYCODE_SLASH :		return EKey::Slash;
			case AKEYCODE_SEMICOLON :	return EKey::Semicolon;
			case AKEYCODE_APOSTROPHE :	return EKey::Apostrophe;
			case AKEYCODE_COMMA :		return EKey::Comma;
			case AKEYCODE_PERIOD :		return EKey::Period;
			case AKEYCODE_TAB :			return EKey::Tab;
			case AKEYCODE_ENTER :		return EKey::Enter;
			case AKEYCODE_SPACE :		return EKey::Space;
			case AKEYCODE_LEFT_BRACKET:	return EKey::LeftBracket;
			case AKEYCODE_RIGHT_BRACKET:return EKey::RightBracket;
			//case AKEYCODE_SHIFT_LEFT:	return ;
			//case AKEYCODE_SHIFT_RIGHT:return ;
			//case AKEYCODE_ALT_LEFT :	return ;
			//case AKEYCODE_ALT_RIGHT :	return ;
			case AKEYCODE_PAGE_UP :		return EKey::PageUp;
			case AKEYCODE_PAGE_DOWN :	return EKey::PageDown;
			case AKEYCODE_HOME :		return EKey::Home;
			case AKEYCODE_DEL :			return EKey::Delete;
			//case AKEYCODE_FORWARD_DEL : return ;
			//case AKEYCODE_MENU :		return ;
			case AKEYCODE_CAPS_LOCK :	return EKey::CapsLock;
			case AKEYCODE_INSERT :		return EKey::Insert;
			//case AKEYCODE_BREAK :		return ;
			case AKEYCODE_SCROLL_LOCK : return EKey::ScrollLock;
			//case AKEYCODE_CTRL_LEFT : return ;
			//case AKEYCODE_CTRL_RIGHT : return ;
			case AKEYCODE_ESCAPE :		return EKey::Escape;
			//case AKEYCODE_BACK :		return ;
			//case AKEYCODE_SOFT_LEFT :	return ;
			//case AKEYCODE_SOFT_RIGHT :return ;
				
			case AKEYCODE_DPAD_LEFT :	return  EKey::ArrowLeft;
			case AKEYCODE_DPAD_UP :		return EKey::ArrowUp;
			case AKEYCODE_DPAD_RIGHT :	return EKey::ArrowRight;
			case AKEYCODE_DPAD_DOWN :	return EKey::ArrowDown;

			case AKEYCODE_F1 :			return EKey::F1;
			case AKEYCODE_F2 :			return EKey::F2;
			case AKEYCODE_F3 :			return EKey::F3;
			case AKEYCODE_F4 :			return EKey::F4;
			case AKEYCODE_F5 :			return EKey::F5;
			case AKEYCODE_F6 :			return EKey::F6;
			case AKEYCODE_F7 :			return EKey::F7;
			case AKEYCODE_F8 :			return EKey::F8;
			case AKEYCODE_F9 :			return EKey::F9;
			case AKEYCODE_F10 :			return EKey::F10;
			case AKEYCODE_F11 :			return EKey::F11;
			case AKEYCODE_F12 :			return EKey::F12;

			//case AKEYCODE_NUMPAD_0 :		return ;
			//case AKEYCODE_NUMPAD_1 :		return ;
			//case AKEYCODE_NUMPAD_2 :		return ;
			//case AKEYCODE_NUMPAD_3 :		return ;
			//case AKEYCODE_NUMPAD_4 :		return ;
			//case AKEYCODE_NUMPAD_5 :		return ;
			//case AKEYCODE_NUMPAD_6 :		return ;
			//case AKEYCODE_NUMPAD_7 :		return ;
			//case AKEYCODE_NUMPAD_8 :		return ;
			//case AKEYCODE_NUMPAD_9 :		return ;
			//case AKEYCODE_NUM_LOCK :		return ;
			//case AKEYCODE_NUMPAD_DIVIDE :	return ;
			//case AKEYCODE_NUMPAD_MULTIPLY :	return ;
			//case AKEYCODE_NUMPAD_SUBTRACT :	return ;
			//case AKEYCODE_NUMPAD_ENTER :	return ;
			//case AKEYCODE_NUMPAD_DOT :		return ;
			//case AKEYCODE_NUMPAD_COMMA :	return ;
			//case AKEYCODE_NUMPAD_ADD :		return ;
			//case AKEYCODE_NUMPAD_EQUALS :	return ;
		}
		return Default;
	}
}
/*
=================================================
	_MapKeyAction
=================================================
*/
namespace {
	EKeyState  _MapKeyAction (jint action)
	{
		constexpr int ACTION_DOWN		= 0;
		constexpr int ACTION_UP			= 1;
		constexpr int ACTION_MULTIPLE	= 2;

		switch ( action )
		{
			case ACTION_DOWN :		return EKeyState::Down;
			case ACTION_MULTIPLE :	return EKeyState::Hold;
			case ACTION_UP :		return EKeyState::Up;
		}
		return EKeyState::Unknown;
	}
}
/*
=================================================
	_MapTouchAction
=================================================
*/
namespace {
	using ETouchAction = InputEventQueueWriter::TouchEvent::EAction;

	ETouchAction  _MapTouchAction (jint action)
	{
		constexpr int ACTION_DOWN		= 0;
		constexpr int ACTION_UP			= 1;
		constexpr int ACTION_MOVE		= 2;
		constexpr int ACTION_CANCEL		= 3;
		constexpr int ACTION_OUTSIDE	= 4;

		switch ( action )
		{
			case ACTION_DOWN :		return ETouchAction::Down;
			case ACTION_UP :		return ETouchAction::Up;
			case ACTION_MOVE :		return ETouchAction::Move;
			case ACTION_CANCEL :	return ETouchAction::Cancel;
			case ACTION_OUTSIDE :	return ETouchAction::Outside;
		}
		return ETouchAction::Unknown;
	}
}

}	// AE::App

#endif	// PLATFORM_ANDROID
