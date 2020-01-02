// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_GLFW

# ifdef PLATFORM_WINDOWS
#  define GLFW_EXPOSE_NATIVE_WIN32
# endif

# include "platform/App/GLFW/WindowGLFW.h"
# include "stl/Platforms/WindowsHeader.h"

# include "GLFW/glfw3.h"
# include "GLFW/glfw3native.h"

namespace AE::App
{
	
/*
=================================================
	constructor
=================================================
*/
	WindowGLFW::WindowGLFW (UniquePtr<IWndListener> listener, TimePoint_t startTime) :
		_listener{ std::move(listener) },
		_appStartTime{ startTime }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	WindowGLFW::~WindowGLFW ()
	{
		EXLOCK( _drCheck );
		_Destroy();
	}
	
/*
=================================================
	Close
=================================================
*/
	void  WindowGLFW::Close ()
	{
		EXLOCK( _drCheck );

		if ( _window )
		{
			glfwSetWindowShouldClose( _window, GLFW_TRUE );
		}
	}
	
/*
=================================================
	GetSurfaceSize
=================================================
*/
	uint2  WindowGLFW::GetSurfaceSize ()
	{
		uint2	result;

		if ( _window )
		{
			glfwGetFramebufferSize( _window, OUT Cast<int>(&result.x), OUT Cast<int>(&result.y) );
		}
		return result;
	}

/*
=================================================
	GetInputEventQueue
=================================================
*/
	InputEventQueue const&  WindowGLFW::GetInputEventQueue ()
	{
		EXLOCK( _drCheck );
		return _eventQueue;
	}
	
/*
=================================================
	GetNative
=================================================
*/
	NativeWindow  WindowGLFW::GetNative ()
	{
		NativeWindow	result;

		if ( _window )
		{
		#ifdef PLATFORM_WINDOWS
			result.hinstance	= ::GetModuleHandle( null );
			result.hwnd			= glfwGetWin32Window( _window );
		#else
		#	error unsupported platform!
		#endif
		}
		return result;
	}

/*
=================================================
	SetSize
=================================================
*/
	void  WindowGLFW::SetSize (const uint2 &size)
	{
		EXLOCK( _drCheck );

		if ( _window )
		{
			glfwSetWindowSize( _window, int(size.x), int(size.y) );
		}
	}
	
/*
=================================================
	SetPosition
=================================================
*/
	void  WindowGLFW::SetPosition (const int2 &pos)
	{
		EXLOCK( _drCheck );

		if ( _window )
		{
			glfwSetWindowPos( _window, pos.x, pos.y );
		}
	}

	void  WindowGLFW::SetPosition (Monitor::ID monitorId, const int2 &pos)
	{
		EXLOCK( _drCheck );

		if ( _window )
		{
			int				count;
			GLFWmonitor**	monitors = glfwGetMonitors( OUT &count );
			
			if ( monitors and int(monitorId) < count )
			{
				int2	monitor_pos;
				glfwGetMonitorPos( monitors[int(monitorId)], OUT &monitor_pos.x, OUT &monitor_pos.y );
				
				glfwSetWindowPos( _window, monitor_pos.x + pos.x, monitor_pos.y + pos.y );
			}
		}
	}
	
/*
=================================================
	SetTitle
=================================================
*/
	void  WindowGLFW::SetTitle (NtStringView title)
	{
		EXLOCK( _drCheck );

		if ( _window )
		{
			glfwSetWindowTitle( _window, title.c_str() );
		}
	}

/*
=================================================
	_Create
=================================================
*/
	bool  WindowGLFW::_Create (const WindowDesc &desc)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( not _window );

		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		
		GLFWmonitor*	monitor	= null;
		int				count;
		GLFWmonitor**	monitors = glfwGetMonitors( OUT &count );
		int2			monitor_pos;

		if ( monitors and int(desc.monitor) < count )
		{
			monitor = monitors[ int(desc.monitor) ];

			int2	size;
			glfwGetMonitorWorkarea( monitor, OUT &monitor_pos.x, OUT &monitor_pos.y, OUT &size.x, OUT &size.y );

			monitor_pos += (size - int2(desc.size)) / 2;
		}


		glfwWindowHint( GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE );
		glfwWindowHint( GLFW_VISIBLE, GLFW_TRUE );
		glfwWindowHint( GLFW_FOCUSED, GLFW_TRUE );
		glfwWindowHint( GLFW_FOCUS_ON_SHOW, GLFW_TRUE );
		glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE );
		glfwWindowHint( GLFW_SCALE_TO_MONITOR, GLFW_TRUE );
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

		_window = glfwCreateWindow( int(desc.size.x),
									int(desc.size.y),
									desc.title.data(),
									desc.fullscreen ? monitor : null,
									null );
		CHECK_ERR( _window );
		
		if ( not desc.fullscreen )
			glfwSetWindowPos( _window, monitor_pos.x, monitor_pos.y );

		glfwSetWindowUserPointer( _window, this );
		glfwSetWindowRefreshCallback( _window, &_GLFW_RefreshCallback );
		glfwSetFramebufferSizeCallback( _window, &_GLFW_ResizeCallback );
		glfwSetKeyCallback( _window, &_GLFW_KeyCallback );
		glfwSetMouseButtonCallback( _window, &_GLFW_MouseButtonCallback );
		glfwSetCursorPosCallback( _window, &_GLFW_CursorPosCallback );
		glfwSetScrollCallback( _window, &_GLFW_MouseWheelCallback );
		glfwSetWindowIconifyCallback( _window, &_GLFW_IconifyCallback );
		
		_SetState( EState::Created );
		_SetState( EState::Started );
		_SetState( EState::InForeground );

		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void  WindowGLFW::_Destroy ()
	{
		EXLOCK( _drCheck );

		if ( not _window )
			return;
		
		_SetState( EState::InBackground );
		_SetState( EState::Stopped );
		_SetState( EState::Destroyed );
		
		glfwDestroyWindow( _window );
		_window = null;

		_listener.reset();
	}

/*
=================================================
	_ProcessMessages
=================================================
*/
	bool  WindowGLFW::_ProcessMessages ()
	{
		EXLOCK( _drCheck );

		if ( not _window )
			return false;

		if ( glfwWindowShouldClose( _window ))
		{
			_Destroy();
			return false;
		}

		_listener->OnUpdate( *this );
		return true;
	}
	
/*
=================================================
	_Timestamp
=================================================
*/
	WindowGLFW::Milliseconds  WindowGLFW::_Timestamp ()
	{
		EXLOCK( _drCheck );
		return std::chrono::duration_cast<Milliseconds>( TimePoint_t::clock::now() - _appStartTime );
	}
	
/*
=================================================
	_SetState
=================================================
*/
	void  WindowGLFW::_SetState (EState newState)
	{
		if ( Abs( int(_wndState) - int(newState) ) != 1 )
			return;

		_wndState = newState;
		
		BEGIN_ENUM_CHECKS();
		switch ( _wndState )
		{
			case EState::Started :
				_listener->OnStart( *this );
				_listener->OnSurfaceCreated( *this );
				break;

			case EState::Stopped :
				_listener->OnSurfaceDestroyed( *this );
				_listener->OnStop( *this );
				break;

			case EState::Created :		_listener->OnCreate( *this );			break;
			case EState::Destroyed :	_listener->OnDestroy( *this );			break;
			case EState::InForeground :	_listener->OnEnterForeground( *this );	break;
			case EState::InBackground :	_listener->OnEnterBackground( *this );	break;
			case EState::Unknown :
			default :					ASSERT( !"unknown state" );				break;
		}
		END_ENUM_CHECKS();
	}

/*
=================================================
	_MapKey
=================================================
*/
	EKey  WindowGLFW::_MapKey (int key)
	{
		switch ( key )
		{
			case GLFW_MOUSE_BUTTON_LEFT :	return EKey::MB_1;
			case GLFW_MOUSE_BUTTON_RIGHT :	return EKey::MB_2;
			case GLFW_MOUSE_BUTTON_MIDDLE :	return EKey::MB_3;
			case GLFW_MOUSE_BUTTON_4 :		return EKey::MB_4;
			case GLFW_MOUSE_BUTTON_5 :		return EKey::MB_5;
			case GLFW_MOUSE_BUTTON_6 :		return EKey::MB_6;
			case GLFW_MOUSE_BUTTON_7 :		return EKey::MB_7;
			case GLFW_MOUSE_BUTTON_8 :		return EKey::MB_8;

			case GLFW_KEY_SPACE :		return EKey::Space;
			case GLFW_KEY_APOSTROPHE :	return EKey::Apostrophe;
			case GLFW_KEY_COMMA :		return EKey::Comma;
			case GLFW_KEY_MINUS :		return EKey::Minus;
			case GLFW_KEY_PERIOD :		return EKey::Period;
			case GLFW_KEY_SLASH :		return EKey::Slash;
			case GLFW_KEY_0 :			return EKey::n0;
			case GLFW_KEY_1 :			return EKey::n1;
			case GLFW_KEY_2 :			return EKey::n2;
			case GLFW_KEY_3 :			return EKey::n3;
			case GLFW_KEY_4 :			return EKey::n4;
			case GLFW_KEY_5 :			return EKey::n5;
			case GLFW_KEY_6 :			return EKey::n6;
			case GLFW_KEY_7 :			return EKey::n7;
			case GLFW_KEY_8 :			return EKey::n8;
			case GLFW_KEY_9 :			return EKey::n9;
			case GLFW_KEY_SEMICOLON :	return EKey::Semicolon;
			case GLFW_KEY_EQUAL :		return EKey::Equal;
			case GLFW_KEY_A :			return EKey::A;
			case GLFW_KEY_B :			return EKey::B;
			case GLFW_KEY_C :			return EKey::C;
			case GLFW_KEY_D :			return EKey::D;
			case GLFW_KEY_E :			return EKey::E;
			case GLFW_KEY_F :			return EKey::F;
			case GLFW_KEY_G :			return EKey::G;
			case GLFW_KEY_H :			return EKey::H;
			case GLFW_KEY_I :			return EKey::I;
			case GLFW_KEY_J :			return EKey::J;
			case GLFW_KEY_K :			return EKey::K;
			case GLFW_KEY_L :			return EKey::L;
			case GLFW_KEY_M :			return EKey::M;
			case GLFW_KEY_N :			return EKey::N;
			case GLFW_KEY_O :			return EKey::O;
			case GLFW_KEY_P :			return EKey::P;
			case GLFW_KEY_Q :			return EKey::Q;
			case GLFW_KEY_R :			return EKey::R;
			case GLFW_KEY_S :			return EKey::S;
			case GLFW_KEY_T :			return EKey::T;
			case GLFW_KEY_U :			return EKey::U;
			case GLFW_KEY_V :			return EKey::V;
			case GLFW_KEY_W :			return EKey::W;
			case GLFW_KEY_X :			return EKey::X;
			case GLFW_KEY_Y :			return EKey::Y;
			case GLFW_KEY_Z :			return EKey::Z;
			case GLFW_KEY_LEFT_BRACKET:	return EKey::LeftBracket;
			case GLFW_KEY_BACKSLASH :	return EKey::Backslash;
			case GLFW_KEY_RIGHT_BRACKET:return EKey::RightBracket;
			case GLFW_KEY_GRAVE_ACCENT:	return EKey::Grave;
			case GLFW_KEY_ESCAPE :		return EKey::Escape;
			case GLFW_KEY_ENTER :		return EKey::Enter;
			case GLFW_KEY_TAB :			return EKey::Tab;
			case GLFW_KEY_BACKSPACE :	return EKey::Backspace;
			case GLFW_KEY_INSERT :		return EKey::Insert;
			case GLFW_KEY_DELETE :		return EKey::Delete;
			case GLFW_KEY_RIGHT :		return EKey::ArrowRight;
			case GLFW_KEY_LEFT :		return EKey::ArrowLeft;
			case GLFW_KEY_DOWN :		return EKey::ArrowDown;
			case GLFW_KEY_UP :			return EKey::ArrowUp;
			case GLFW_KEY_PAGE_UP :		return EKey::PageUp;
			case GLFW_KEY_PAGE_DOWN :	return EKey::PageDown;
			case GLFW_KEY_HOME :		return EKey::Home;
			case GLFW_KEY_END :			return EKey::End;
			case GLFW_KEY_CAPS_LOCK :	return EKey::CapsLock;
			case GLFW_KEY_SCROLL_LOCK :	return EKey::ScrollLock;
			case GLFW_KEY_NUM_LOCK :	return EKey::NumLock;
			case GLFW_KEY_PRINT_SCREEN:	return EKey::PrintScreen;
			case GLFW_KEY_PAUSE :		return EKey::Pause;
			case GLFW_KEY_F1 :			return EKey::F1;
			case GLFW_KEY_F2 :			return EKey::F2;
			case GLFW_KEY_F3 :			return EKey::F3;
			case GLFW_KEY_F4 :			return EKey::F4;
			case GLFW_KEY_F5 :			return EKey::F5;
			case GLFW_KEY_F6 :			return EKey::F6;
			case GLFW_KEY_F7 :			return EKey::F7;
			case GLFW_KEY_F8 :			return EKey::F8;
			case GLFW_KEY_F9 :			return EKey::F9;
			case GLFW_KEY_F10 :			return EKey::F10;
			case GLFW_KEY_F11 :			return EKey::F11;
			case GLFW_KEY_F12 :			return EKey::F12;
		}
		return Default;
	}

/*
=================================================
	_GLFW_RefreshCallback
=================================================
*/
	void  WindowGLFW::_GLFW_RefreshCallback (GLFWwindow* wnd)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		//for (auto& listener : self->_listeners) {
		//	listener->OnRefresh();
		//}
	}
	
/*
=================================================
	_GLFW_ResizeCallback
=================================================
*/
	void  WindowGLFW::_GLFW_ResizeCallback (GLFWwindow* wnd, int w, int h)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		EXLOCK( self->_drCheck );
		self->_listener->OnResize( *self, uint2{w, h} );
	}

/*
=================================================
	_GLFW_KeyCallback
=================================================
*/
	void  WindowGLFW::_GLFW_KeyCallback (GLFWwindow* wnd, int key, int, int action, int)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));

		InputEventQueue::KeyEvent	ev;
		ev.key		= _MapKey( key );
		ev.state	= (action == GLFW_PRESS   ? EKeyState::Down :
					   action == GLFW_RELEASE ? EKeyState::Up   :
												EKeyState::Hold);
		ev.timestamp = self->_Timestamp();

		self->_eventQueue.Push( ev );
	}
	
/*
=================================================
	_GLFW_MouseButtonCallback
=================================================
*/
	void  WindowGLFW::_GLFW_MouseButtonCallback (GLFWwindow* wnd, int button, int action, int)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		InputEventQueue::KeyEvent	ev;
		ev.key		= _MapKey( button );
		ev.state	= (action == GLFW_PRESS   ? EKeyState::Down :
					   action == GLFW_RELEASE ? EKeyState::Up   :
												EKeyState::Hold);
		ev.timestamp = self->_Timestamp();

		self->_eventQueue.Push( ev );
	}
	
/*
=================================================
	_GLFW_CursorPosCallback
=================================================
*/
	void  WindowGLFW::_GLFW_CursorPosCallback (GLFWwindow* wnd, double xpos, double ypos)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		InputEventQueue::MouseEvent	ev;
		ev.pos		 = { int(xpos), int(ypos) };
		ev.timestamp = self->_Timestamp();

		self->_eventQueue.Push( ev );
	}
	
/*
=================================================
	_GLFW_MouseWheelCallback
=================================================
*/
	void  WindowGLFW::_GLFW_MouseWheelCallback (GLFWwindow* wnd, double, double dy)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));
		
		InputEventQueue::KeyEvent	ev;
		ev.key		= dy > 0.0 ? EKey::MouseWheelUp : EKey::MouseWheelDown;
		ev.state	= EKeyState::Up;
		ev.timestamp = self->_Timestamp();

		self->_eventQueue.Push( ev );
	}
	
/*
=================================================
	_GLFW_IconifyCallback
=================================================
*/
	void  WindowGLFW::_GLFW_IconifyCallback (GLFWwindow* wnd, int iconified)
	{
		auto*	self = static_cast<WindowGLFW *>(glfwGetWindowUserPointer( wnd ));

		if ( iconified == GLFW_TRUE )
			self->_SetState( EState::InBackground );
		else
			self->_SetState( EState::InForeground );
	}


}	// AE::App

#endif	// AE_ENABLE_GLFW
