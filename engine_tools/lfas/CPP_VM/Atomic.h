// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "CPP_VM/VirtualMachine.h"

namespace LFAS::CPP
{
	
	enum class EMemoryOrder
	{
		Acquire,
		Release,
		AcquireRelase,
		Relaxed,
		SequentiallyConsistent,
	};

	void ThreadFence (EMemoryOrder);
	void CompilerBarrier (EMemoryOrder);



	//
	// Atomic
	//

	template <typename T>
	class Atomic
	{
		STATIC_ASSERT( std::is_trivially_copyable_v<T> );
		STATIC_ASSERT( std::is_copy_constructible_v<T> );
		STATIC_ASSERT( std::is_move_constructible_v<T> );
		STATIC_ASSERT( std::is_copy_assignable_v<T> );
		STATIC_ASSERT( std::is_move_assignable_v<T> );

	// types
	public:
		using value_type		= T;
		
		static constexpr bool	is_always_lock_free = true;


	// variables
	private:
		std::shared_mutex	_guard;
		T					_value;


	// methods
	public:
		explicit Atomic (T initial = T(0));
		~Atomic ();

		Atomic (const Atomic<T> &) = delete;
		Atomic (Atomic<T> &&) = delete;

		void operator = (const Atomic<T> &) = delete;
		void operator = (Atomic<T> &&) = delete;


		void  store (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);

		ND_ T  load (EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);

		T  exchange (T newValue, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);
		
		bool  compare_exchange_weak (INOUT T &expected, T desired, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);
		bool  compare_exchange_strong (INOUT T &expected, T desired, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);

		bool  compare_exchange_weak (INOUT T &expected, T desired, EMemoryOrder success, EMemoryOrder failure);
		bool  compare_exchange_strong (INOUT T &expected, T desired, EMemoryOrder success, EMemoryOrder failure);

		T  fetch_add (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);
		T  fetch_sub (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);

		T  fetch_and (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);
		T  fetch_or  (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);
		T  fetch_xor (T val, EMemoryOrder order = EMemoryOrder::SequentiallyConsistent);

	private:
		static void _Fence (VirtualMachine &vm, EMemoryOrder order);
	};

	

/*
=================================================
	ThreadFence
----
	emulates 'std::atomic_thread_fence'
=================================================
*/
	inline void  ThreadFence (EMemoryOrder order)
	{
		VirtualMachine&	vm = VirtualMachine::Instance();
		BEGIN_ENUM_CHECKS();
		switch ( order )
		{
			case EMemoryOrder::Acquire :				vm.ThreadFenceAcquire();			break;
			case EMemoryOrder::Release :				vm.ThreadFenceRelease();			break;
			case EMemoryOrder::SequentiallyConsistent :
			case EMemoryOrder::AcquireRelase :			vm.ThreadFenceAcquireRelease();		break;
			case EMemoryOrder::Relaxed :				vm.ThreadFenceRelaxed();			break;
			default :									CHECK( !"unknown memory order" );	break;
		}
		END_ENUM_CHECKS();

		vm.Yield();
	}

/*
=================================================
	CompilerFence
----
	emulates 'std::atomic_signal_fence'
=================================================
*/
	inline void  CompilerFence (EMemoryOrder)
	{
		CHECK( !"TODO" );
	}
//-----------------------------------------------------------------------------

	
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline Atomic<T>::Atomic (T initial) :
		_value{ initial }
	{
		EXLOCK( _guard );
		VirtualMachine::Instance().AtomicCreate( &_value );
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename T>
	inline Atomic<T>::~Atomic ()
	{
		EXLOCK( _guard );
		VirtualMachine::Instance().AtomicDestroy( &_value );
	}

/*
=================================================
	store
=================================================
*/
	template <typename T>
	inline void  Atomic<T>::store (T val, EMemoryOrder order)
	{
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );

			_value = val;
			_Fence( vm, order );
		}
		vm.Yield();
	}

/*
=================================================
	load
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::load (EMemoryOrder order)
	{
		T		result;
		auto&	vm		= VirtualMachine::Instance();
		{
			SHAREDLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );

			result = _value;
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	exchange
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::exchange (T newValue, EMemoryOrder order)
	{
		T		result;
		auto&	vm		= VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );

			result = _value;
			_value = newValue;

			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	compare_exchange_weak
=================================================
*/
	template <typename T>
	inline bool  Atomic<T>::compare_exchange_weak (INOUT T &expected, T desired, EMemoryOrder order)
	{
		return compare_exchange_weak( INOUT expected, desired, order, EMemoryOrder::Relaxed );
	}

	template <typename T>
	inline bool  Atomic<T>::compare_exchange_weak (INOUT T &expected, const T desired, EMemoryOrder success, EMemoryOrder failure)
	{
		auto&	vm		= VirtualMachine::Instance();
		bool	result	= true;
		do
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( (success == EMemoryOrder::SequentiallyConsistent) or
														  (failure == EMemoryOrder::SequentiallyConsistent) );
			bool	is_equal	= (expected == _value);
			
			expected = _value;

			if ( not is_equal or vm.AtomicCompareExchangeWeakFalsePositive( &_value ))
			{
				_Fence( vm, failure );
				result = false;
				break;
			}

			_value = desired;
			_Fence( vm, success );

		} while (0);

		vm.Yield();
		return result;
	}

/*
=================================================
	compare_exchange_strong
=================================================
*/
	template <typename T>
	inline bool  Atomic<T>::compare_exchange_strong (INOUT T &expected, T desired, EMemoryOrder order)
	{
		return compare_exchange_strong( INOUT expected, desired, order, EMemoryOrder::Relaxed );
	}

	template <typename T>
	inline bool  Atomic<T>::compare_exchange_strong (INOUT T &expected, const T desired, EMemoryOrder success, EMemoryOrder failure)
	{
		auto&	vm		= VirtualMachine::Instance();
		bool	result	= true;
		do
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( (success == EMemoryOrder::SequentiallyConsistent) or
														  (failure == EMemoryOrder::SequentiallyConsistent) );
			bool	is_equal	= (expected == _value);
			
			expected = _value;

			if ( not is_equal )
			{
				_Fence( vm, failure );
				result = false;
				break;
			}

			_value = desired;
			_Fence( vm, success );

		} while (0);
		
		vm.Yield();
		return result;
	}

/*
=================================================
	fetch_add
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::fetch_add (const T val, EMemoryOrder order)
	{
		STATIC_ASSERT( IsInteger<T> );

		T		result;
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );
		
			result  = _value;
			_value += val;
			
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	fetch_sub
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::fetch_sub (const T val, EMemoryOrder order)
	{
		STATIC_ASSERT( IsInteger<T> );

		T		result;
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );
		
			result  = _value;
			_value -= val;
			
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	fetch_and
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::fetch_and (const T val, EMemoryOrder order)
	{
		STATIC_ASSERT( IsInteger<T> );

		T		result;
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );
		
			result  = _value;
			_value &= val;
			
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	fetch_or
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::fetch_or (const T val, EMemoryOrder order)
	{
		STATIC_ASSERT( IsInteger<T> );

		T		result;
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );
		
			result  = _value;
			_value |= val;
			
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}

/*
=================================================
	fetch_xor
=================================================
*/
	template <typename T>
	inline T  Atomic<T>::fetch_xor (const T val, EMemoryOrder order)
	{
		STATIC_ASSERT( IsInteger<T> );

		T		result;
		auto&	vm = VirtualMachine::Instance();
		{
			EXLOCK( _guard );
			auto	global_lock = vm.GetAtomicGlobalLock( order == EMemoryOrder::SequentiallyConsistent );
		
			result  = _value;
			_value ^= val;
			
			_Fence( vm, order );
		}
		vm.Yield();
		return result;
	}
	
/*
=================================================
	_Fence
=================================================
*/
	template <typename T>
	inline void  Atomic<T>::_Fence (VirtualMachine &vm, EMemoryOrder order)
	{
		BEGIN_ENUM_CHECKS();
		switch ( order )
		{
			case EMemoryOrder::Acquire :				vm.ThreadFenceAcquire();			break;
			case EMemoryOrder::Release :				vm.ThreadFenceRelease();			break;
			case EMemoryOrder::SequentiallyConsistent :
			case EMemoryOrder::AcquireRelase :			vm.ThreadFenceAcquireRelease();		break;
			case EMemoryOrder::Relaxed :				vm.ThreadFenceRelaxed();			break;
			default :									CHECK( !"unknown memory order" );	break;
		}
		END_ENUM_CHECKS();
	}


}	// LFAS::CPP
