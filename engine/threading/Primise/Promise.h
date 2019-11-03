// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/CompileTime/FunctionInfo.h"

namespace AE::Threading
{

	//
	// Promise
	//

	template <typename T>
	class Promise final
	{
	// types
	public:
		using Value_t	= T;
		using Self		= Promise< T >;
		
		class _InternalImpl;
		using _InternalImplPtr = SharedPtr< _InternalImpl >;


	// variables
	private:
		_InternalImplPtr	_impl;


	// methods
	public:
		Promise () = delete;

		Promise (Self &&) = default;
		Promise (const Self &) = default;

		Self& operator = (Self &&) = default;
		Self& operator = (const Self &) = default;

		template <typename Fn>
		auto  Then (Fn &&fn);

		template <typename Fn>
		auto  Except (Fn &&fn);

		bool  Cancel ();

		explicit operator AsyncTask () const	{ return _impl; }
		
	private:
		template <typename Fn>
		Promise (Fn &&fn, bool except, Array<AsyncTask> &&dependsOn);

		template <typename Fn>
		friend auto  MakePromise (Fn &&fn, Array<AsyncTask> &&dependsOn);

		template <typename ...Types>
		friend auto  MakePromiseFromTuple (const Tuple<Types...> &t);
		
		template <typename B>
		friend class Promise;
	};


	//
	// Promise implementation
	//

	template <typename T>
	class Promise<T>::_InternalImpl final : public IAsyncTask
	{
	// types
	private:
		using Result_t = Conditional< IsVoid<T>, bool, Optional<T> >;

	// variables
	private:
		Function< T () >	_func;
		Result_t			_result;
		const bool			_isExept;


	// methods
	public:
		template <typename Fn>
		_InternalImpl (Fn &&fn, bool except);

		ND_ auto  Result () const
		{
			ThreadFence( memory_order_acquire );

			if constexpr( not IsVoid<T> )
				return *_result;
		}

	private:
		void Run () override;
		void OnCancel () override;
	};
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline Promise<T>::Promise (Fn &&fn, bool except, Array<AsyncTask> &&dependsOn) :
		_impl{ Cast<_InternalImpl>( Scheduler().Run<_InternalImpl>( std::move(dependsOn), std::forward<Fn>(fn), except ))}
	{}

/*
=================================================
	Then
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline auto  Promise<T>::Then (Fn &&fn)
	{
		using FI		= FunctionInfo< Fn >;
		using Result	= Promise< typename FI::result >;

		if constexpr( IsVoid<T> )
		{
			STATIC_ASSERT( FI::args::Count == 0 );
		
			return Result{ fn, false, Array<AsyncTask>{_impl} };
		}
		else
		{
			STATIC_ASSERT( FI::args::Count == 1 );
			STATIC_ASSERT( IsSameTypes< FI::args::Get<0>, const T& >);

			return Result{	[fn = std::forward<Fn>(fn), in = _impl] () {
								return fn( in->Result() );
							},
							false,
							Array<AsyncTask>{_impl} };
		}
	}
	
/*
=================================================
	Except
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline auto  Promise<T>::Except (Fn &&fn)
	{
		using FI		= FunctionInfo< Fn >;
		using Result	= Promise< typename FI::result >;
		
		if constexpr( IsVoid<T> )
		{
			STATIC_ASSERT( FI::args::Count == 0 );
		
			return Result{ fn, true, Array<AsyncTask>{_impl} };
		}
		else
		{
			STATIC_ASSERT( FI::args::Count == 1 );
			STATIC_ASSERT( IsSameTypes< FI::args::Get<0>, const T& >);

			return Result{	[fn = std::forward<Fn>(fn), in = _impl] () {
								return fn( in->Result() );
							},
							true,
							Array<AsyncTask>{_impl} };
		}
	}
	
/*
=================================================
	Cancel
=================================================
*/
	template <typename T>
	inline bool  Promise<T>::Cancel ()
	{
		return Scheduler().Cancel( _impl );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline Promise<T>::_InternalImpl::_InternalImpl (Fn &&fn, bool except) :
		IAsyncTask{ EThread::Worker },
		_func{ std::forward<Fn>(fn) },
		_isExept{ except }
	{
		ASSERT( _func );
	}
	
/*
=================================================
	Run
=================================================
*/
	template <typename T>
	inline void  Promise<T>::_InternalImpl::Run ()
	{
		if ( not _isExept and _func )
		{
			if constexpr( IsVoid<T> )
				_func();
			else
				_result	= _func();

			_func = null;
		}
	}
	
/*
=================================================
	OnCancel
=================================================
*/
	template <typename T>
	inline void  Promise<T>::_InternalImpl::OnCancel ()
	{
		if ( _isExept and _func )
		{
			if constexpr( IsVoid<T> )
				_func();
			else
				_result	= _func();

			_func = null;
		}
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	MakePromise
=================================================
*/
	template <typename Fn>
	forceinline auto  MakePromise (Fn &&fn, Array<AsyncTask> &&dependsOn = {})
	{
		STATIC_ASSERT( std::is_invocable_v<Fn> );

		using Result = Promise< typename FunctionInfo<Fn>::result >;
		return Result{ std::forward<Fn>(fn), false, std::move(dependsOn) };
	}
	
/*
=================================================
	MakePromiseFromTuple
=================================================
*/
	template <typename ...Types>
	forceinline auto  MakePromiseFromTuple (const Tuple<Types...> &t)
	{
		return MakePromise(
			[t] () {
				return std::apply( [] (auto&& ...args) {
						return std::make_tuple( args._impl->Result() ... );
					}, t );
			},
			std::apply( [] (auto&& ...args) { return Array<AsyncTask>{ AsyncTask{std::forward<decltype(args)>( args )}... };}, t )
		);
	}

}	// AE::Threading
