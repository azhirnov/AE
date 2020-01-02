// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "platform/App/Android/Java.h"

#ifdef PLATFORM_ANDROID

namespace AE::Java
{
	
/*
=================================================
	GetJavaVM
=================================================
*/
namespace {
	ND_ JavaVM*&  GetJavaVM ()
	{
		static JavaVM*	jvm = null;
		return jvm;
	}
}

/*
=================================================
	constructor
=================================================
*/
	JavaEnv::JavaEnv (JNIEnv *env) : _env{ env }
	{
		Attach();
	}
	
	JavaEnv::JavaEnv ()
	{
		Attach();
	}
	
/*
=================================================
	destructor
=================================================
*/
	JavaEnv::~JavaEnv ()
	{
		Detach();
	}
	
/*
=================================================
	Attach
=================================================
*/
	void JavaEnv::Attach ()
	{
		if ( _env )
			return;	// already attached

		JavaVM*	jvm = GetJavaVM();
		if ( not jvm )
		{
			ASSERT( !"JavaVM is null" );
			return;
		}

		void*	env		= null;
		jint	status	= jvm->GetEnv( OUT &env, Version );

		_env = Cast<JNIEnv>( env );

		if ( status == JNI_OK and _env )
		{
			_mustBeDetached = false;
			return;
		}

		if ( status == JNI_EDETACHED )
		{
			if ( jvm->AttachCurrentThread( &_env, null ) < 0 )
			{
				ASSERT( !"can't attach to current java thread" );
				return;
			}
			_mustBeDetached = true;
			return;
		}

		ASSERT( !"unknown error" );
	}
	
/*
=================================================
	Detach
=================================================
*/
	void JavaEnv::Detach ()
	{
		if ( _mustBeDetached )
		{
			JavaVM*	jvm = GetJavaVM();
			if ( not jvm )
			{
				ASSERT( !"JavaVM is null" );
				return;
			}

			jvm->DetachCurrentThread();

			_env			= null;
			_mustBeDetached	= false;
		}
	}
	
/*
=================================================
	SetVM
=================================================
*/
	void JavaEnv::SetVM (JavaVM *ptr)
	{
		ASSERT( GetJavaVM() == null );

		GetJavaVM() = ptr;
	}
	
/*
=================================================
	ThrowException
=================================================
*/
	void JavaEnv::ThrowException (NtStringView msg) const
	{
		CHECK_ERR( _env, void());

		JavaClass	jc{ "java/lang/Error" };
		_env->ThrowNew( jc.Get(), msg.c_str() );
	}
	
/*
=================================================
	ExceptionClear
=================================================
*/
	void JavaEnv::ExceptionClear () const
	{
		CHECK_ERR( _env, void());

		_env->ExceptionClear();
	}
	
/*
=================================================
	HasException
=================================================
*/
	bool JavaEnv::HasException () const
	{
		CHECK_ERR( _env, false );

		return _env->ExceptionCheck() == JNI_TRUE;
	}
	

}	// AE::Java

#endif	// PLATFORM_ANDROID
