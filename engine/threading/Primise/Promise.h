// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/TaskSystem/TaskScheduler.h"
#include "stl/CompileTime/FunctionInfo.h"

namespace AE::Threading
{
	struct PromiseNullResult {};


	//
	// Promise Result
	//

	template <typename T>
	struct PromiseResult
	{
		STATIC_ASSERT( not IsSameTypes< PromiseNullResult, T >);

	// variables
	private:
		union {
			T			_value;
			uint8_t		_data [ sizeof(T) ];	// don't use it!
		};
		bool			_hasValue	= false;


	// methods
	public:
		PromiseResult () = delete;

		PromiseResult (T &&value) : _value{std::move(value)}, _hasValue{true} {}
		PromiseResult (const T &value) : _value{value}, _hasValue{true} {}
		PromiseResult (const PromiseNullResult &) {}

		PromiseResult (const PromiseResult<T> &other);
		PromiseResult (PromiseResult<T> &&other);

		PromiseResult<T>&  operator = (const PromiseResult<T> &rhs);
		PromiseResult<T>&  operator = (PromiseResult<T> &&rhs);

		~PromiseResult ()					{ if ( _hasValue ) _value.~T(); }

		ND_ bool		HasValue ()	const	{ return _hasValue; }
		ND_ T const&	Value ()	const	{ ASSERT( _hasValue );  return _value; }
		ND_ T &			Value ()			{ ASSERT( _hasValue );  return _value; }
	};

	
	template <>
	struct PromiseResult< void >
	{
	// variables
	private:
		bool		_hasValue	= false;

	// methods
	public:
		PromiseResult () : _hasValue{true} {}

		PromiseResult (const PromiseNullResult &) {}

		PromiseResult (const PromiseResult<void> &) = default;
		PromiseResult (PromiseResult<void> &&) = default;
		
		PromiseResult<void>&  operator = (const PromiseResult<void> &) = default;
		PromiseResult<void>&  operator = (PromiseResult<void> &&) = default;

		ND_ bool	HasValue ()	const	{ return _hasValue; }
	};



	//
	// Promise
	//

	template <typename T>
	class Promise final
	{
		STATIC_ASSERT( not IsSameTypes< PromiseNullResult, T >);

	// types
	public:
		using Value_t	= T;
		using Self		= Promise< T >;
		using EThread	= IAsyncTask::EThread;
		
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
		auto  Then (Fn &&fn, EThread thread = EThread::Worker);

		template <typename Fn>
		auto  Except (Fn &&fn, EThread thread = EThread::Worker);

		bool  Cancel ();

		explicit operator AsyncTask () const	{ return _impl; }
		
	private:
		template <typename Fn, typename Deps>
		auto  _Then (Fn &&fn, Deps &&dependsOn, EThread thread);

		template <typename Fn, typename Deps>
		Promise (Fn &&fn, bool except, Deps &&dependsOn, EThread thread);

		template <typename Fn, typename Deps>
		friend auto  MakePromise (Fn &&fn, Deps &&dependsOn, IAsyncTask::EThread thread);

		template <typename ...Types>
		friend auto  MakePromiseFromTuple (const Tuple<Types...> &t, IAsyncTask::EThread thread);
		
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
		using Result_t	= PromiseResult< T >;
		using Func_t	= Function< PromiseResult<T> () >;


	// variables
	private:
		Func_t			_func;
		Result_t		_result;
		const bool		_isExept;


	// methods
	public:
		template <typename Fn>
		_InternalImpl (Fn &&fn, bool except, EThread thread);

		ND_ auto  Result () const
		{
			ASSERT( Status() == EStatus::Completed );
			ThreadFence( memory_order_acquire );

			if constexpr( not IsVoid<T> )
				return _result.Value();
		}

		ND_ bool  IsExcept () const	{ return _isExept; }

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
	inline PromiseResult<T>::PromiseResult (const PromiseResult<T> &other) :
		_hasValue{ other._hasValue }
	{
		if ( _hasValue )
			PlacementNew<T>( &_value, other._value );
	}

	template <typename T>
	inline PromiseResult<T>::PromiseResult (PromiseResult<T> &&other) :
		_hasValue{ other._hasValue }
	{
		if ( _hasValue )
			PlacementNew<T>( &_value, std::move(other._value) );
	}

/*
=================================================
	operator =
=================================================
*/
	template <typename T>
	inline PromiseResult<T>&  PromiseResult<T>::operator = (const PromiseResult<T> &rhs)
	{
		if ( _hasValue )
			_value.~T();

		_hasValue = rhs._hasValue;

		if ( _hasValue )
			PlacementNew<T>( &_value, rhs._value );

		return *this;
	}
	
	template <typename T>
	inline PromiseResult<T>&  PromiseResult<T>::operator = (PromiseResult<T> &&rhs)
	{
		if ( _hasValue )
			_value.~T();

		_hasValue = rhs._hasValue;

		if ( _hasValue )
		{
			PlacementNew<T>( &_value, std::move(rhs._value) );
			rhs._hasValue = false;
		}
		return *this;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	ResultToPromise
=================================================
*/
namespace _ae_threading_hidden_
{
	template <typename T>
	struct ResultToPromise {
		using type = Promise< T >;
	};

	template <typename T>
	struct ResultToPromise< PromiseResult<T> > {
		using type = Promise< T >;
	};

	template <>
	struct ResultToPromise< PromiseNullResult > {
		using type = Promise< void >;
	};

}	// _ae_threading_hidden_
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	template <typename Fn, typename Deps>
	inline Promise<T>::Promise (Fn &&fn, bool except, Deps &&dependsOn, EThread thread) :
		_impl{ Cast<_InternalImpl>( Scheduler().Run<_InternalImpl>( std::move(dependsOn), std::forward<Fn>(fn), except, thread ))}
	{}

/*
=================================================
	Then
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline auto  Promise<T>::Then (Fn &&fn, EThread thread)
	{
		if ( _impl->IsExcept() )
			return _Then( std::forward<Fn>(fn), WeakDeps{_impl}, thread );
		else
			return _Then( std::forward<Fn>(fn), StrongDeps{_impl}, thread );
	}
	
/*
=================================================
	_Then
=================================================
*/
	template <typename T>
	template <typename Fn, typename Deps>
	inline auto  Promise<T>::_Then (Fn &&fn, Deps &&dependsOn, EThread thread)
	{
		using FI		= FunctionInfo< Fn >;
		using Result	= typename _ae_threading_hidden_::ResultToPromise< typename FI::result >::type;

		if constexpr( IsVoid<T> and IsVoid< typename FI::result > )
		{
			STATIC_ASSERT( FI::args::Count == 0 );
		
			return Result{	[fn = std::forward<Fn>(fn)] () {
								fn();
								return PromiseResult<void>{};
							},
							false,
							std::move(dependsOn),
							thread };
		}
		else
		if constexpr( IsVoid<T> )
		{
			STATIC_ASSERT( FI::args::Count == 0 );
		
			return Result{ std::forward<Fn>(fn), false, std::move(dependsOn), thread };
		}
		else
		if constexpr( IsVoid< typename FI::result > )
		{
			STATIC_ASSERT( FI::args::Count == 1 );
			STATIC_ASSERT( IsSameTypes< FI::args::Get<0>, const T& >);

			return Result{	[fn = std::forward<Fn>(fn), in = _impl] () {
								fn( in->Result() );
								return PromiseResult<void>{};
							},
							false,
							std::move(dependsOn),
							thread };
		}
		else
		{
			STATIC_ASSERT( FI::args::Count == 1 );
			STATIC_ASSERT( IsSameTypes< FI::args::Get<0>, const T& >);

			return Result{	[fn = std::forward<Fn>(fn), in = _impl] () {
								return fn( in->Result() );
							},
							false,
							std::move(dependsOn),
							thread };
		}
	}

/*
=================================================
	Except
=================================================
*/
	template <typename T>
	template <typename Fn>
	inline auto  Promise<T>::Except (Fn &&fn, EThread thread)
	{
		using FI		= FunctionInfo< Fn >;
		using Result	= typename _ae_threading_hidden_::ResultToPromise< typename FI::result >::type;
		
		STATIC_ASSERT( FI::args::Count == 0 );
		
		if constexpr( IsVoid< typename FI::result > )
		{
			return Result{	[fn = std::forward<Fn>(fn)] () {
								fn();
								return PromiseResult<void>{};
							},
							true,
							StrongDeps{_impl},
							thread };
		}
		else
		{
			return Result{ std::forward<Fn>(fn), true, StrongDeps{_impl} };
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
	inline Promise<T>::_InternalImpl::_InternalImpl (Fn &&fn, bool except, EThread thread) :
		IAsyncTask{ thread },
		_func{ std::forward<Fn>(fn) },
		_result{ PromiseNullResult{} },
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
			_result	= _func();
			_func	= null;

			// set failed state
			if ( not _result.HasValue() )
			{
				_SetFailedState();
			}
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
			_result	= _func();
			_func	= null;
		}
	}
//-----------------------------------------------------------------------------


/*
=================================================
	MakePromise
=================================================
*/
	template <typename Fn, typename Deps = StrongDeps>
	forceinline auto  MakePromise (Fn &&fn, Deps &&dependsOn = Default, IAsyncTask::EThread thread = IAsyncTask::EThread::Worker)
	{
		STATIC_ASSERT( std::is_invocable_v<Fn> );

		using Value_t	= typename FunctionInfo< Fn >::result;
		using Result	= typename _ae_threading_hidden_::ResultToPromise< Value_t >::type;

		if constexpr( IsVoid< Value_t > )
		{
			return Result{	[fn = std::forward<Fn>(fn)] () {
								fn();
								return PromiseResult<void>{};
							},
							false,
							std::move(dependsOn),
							thread };
		}
		else
		{
			return Result{ std::forward<Fn>(fn), false, std::move(dependsOn), thread };
		}
	}
	
/*
=================================================
	MakePromiseFromTuple
=================================================
*/
	template <typename ...Types>
	forceinline auto  MakePromiseFromTuple (const Tuple<Types...> &t, IAsyncTask::EThread thread = IAsyncTask::EThread::Worker)
	{
		return MakePromise(
			[t] () {
				return std::apply( [] (auto&& ...args) {
						return std::make_tuple( args._impl->Result() ... );
					}, t );
			},
			std::apply( [] (auto&& ...args) {
				return StrongDeps{ AsyncTask{std::forward<decltype(args)>( args )}... };
			}, t ),
			thread
		);
	}

}	// AE::Threading
