// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"

#include <shared_mutex>

#ifdef AE_ENABLE_DATA_RACE_CHECK

#include <atomic>
#include <mutex>
#include <thread>

namespace AE::Threading
{

	//
	// Data Race Check
	//

	struct DataRaceCheck
	{
	// variables
	private:
		mutable Atomic<size_t>		_state  { 0 };


	// methods
	public:
		DataRaceCheck () {}

		ND_ bool  Lock () const
		{
			const size_t	id		= size_t(HashOf( std::this_thread::get_id() ));
			size_t			curr	= _state.load( memory_order_acquire );
			
			if ( curr == id )
				return true;	// recursive lock

			curr	= 0;
			bool	locked	= _state.compare_exchange_strong( INOUT curr, id, memory_order_relaxed );
			CHECK_ERR( curr == 0 );		// locked by another thread - race condition detected!
			CHECK_ERR( locked );
			return true;
		}

		void  Unlock () const
		{
			_state.store( 0, memory_order_relaxed );
		}
	};



	//
	// Read/Write Data Race Check
	//

	struct RWDataRaceCheck
	{
	// variables
	private:
		mutable std::recursive_mutex	_lockWrite;
		mutable Atomic<int>				_readCounter { 0 };


	// methods
	public:
		RWDataRaceCheck () {}


		ND_ bool  LockExclusive ()
		{
			bool	locked = _lockWrite.try_lock();
			CHECK_ERR( locked );	// locked by another thread - race condition detected!

			int		expected = _readCounter.load( memory_order_acquire );
			CHECK_ERR( expected <= 0 );	// has read lock(s) - race condition detected!

			_readCounter.compare_exchange_strong( INOUT expected, expected-1, memory_order_relaxed );
			CHECK_ERR( expected <= 0 );	// has read lock(s) - race condition detected!
			return true;
		}

		void  UnlockExclusive ()
		{
			_readCounter.fetch_add( 1, memory_order_relaxed );
			_lockWrite.unlock();
		}


		ND_ bool  LockShared () const
		{
			int		expected	= 0;
			bool	locked		= false;
			do {
				locked = _readCounter.compare_exchange_weak( INOUT expected, expected+1, memory_order_relaxed );
			
				// if has exclusive lock in current thread
				if ( expected < 0 and _lockWrite.try_lock() )
				{
					_lockWrite.unlock();
					return false;	// don't call 'UnlockShared'
				}
				CHECK_ERR( expected >= 0 );	// has write lock(s) - race condition detected!
			}
			while ( not locked );

			return true;
		}

		void  UnlockShared () const
		{
			_readCounter.fetch_sub( 1, memory_order_relaxed );
		}
	};

}	// AE::Threading

namespace std
{
	template <>
	struct unique_lock< AE::Threading::DataRaceCheck >
	{
	private:
		AE::Threading::DataRaceCheck &	_lock;
		bool							_locked	= false;

	public:
		explicit unique_lock (AE::Threading::DataRaceCheck &ref) : _lock{ref}
		{
			_locked = _lock.Lock();
		}

		unique_lock (const unique_lock &) = delete;
		unique_lock (unique_lock &&) = delete;

		~unique_lock ()
		{
			if ( _locked )
				_lock.Unlock();
		}
	};

	template <>
	struct unique_lock< const AE::Threading::DataRaceCheck > :
		unique_lock< AE::Threading::DataRaceCheck >
	{
		explicit unique_lock (const AE::Threading::DataRaceCheck &ref) :
			unique_lock< AE::Threading::DataRaceCheck >{ const_cast<AE::Threading::DataRaceCheck &>(ref) }
		{}
	};


	template <>
	struct unique_lock< AE::Threading::RWDataRaceCheck >
	{
	private:
		AE::Threading::RWDataRaceCheck &	_lock;
		bool								_locked	= false;

	public:
		explicit unique_lock (AE::Threading::RWDataRaceCheck &ref) : _lock{ref}
		{
			_locked = _lock.LockExclusive();
		}

		unique_lock (const unique_lock &) = delete;
		unique_lock (unique_lock &&) = delete;

		~unique_lock ()
		{
			if ( _locked )
				_lock.UnlockExclusive();
		}
	};

	template <>
	struct unique_lock< const AE::Threading::RWDataRaceCheck > :
		unique_lock< AE::Threading::RWDataRaceCheck >
	{
		explicit unique_lock (const AE::Threading::RWDataRaceCheck &ref) :
			unique_lock< AE::Threading::RWDataRaceCheck >{ const_cast<AE::Threading::RWDataRaceCheck &>(ref) }
		{}
	};


	template <>
	struct shared_lock< AE::Threading::RWDataRaceCheck >
	{
	private:
		AE::Threading::RWDataRaceCheck &	_lock;
		bool								_locked	= false;

	public:
		explicit shared_lock (AE::Threading::RWDataRaceCheck &ref) : _lock{ref}
		{
			_locked = _lock.LockShared();
		}

		shared_lock (const shared_lock &) = delete;
		shared_lock (shared_lock &&) = delete;

		~shared_lock ()
		{
			if ( _locked )
				_lock.UnlockShared();
		}
	};

	template <>
	struct shared_lock< const AE::Threading::RWDataRaceCheck > :
		shared_lock< AE::Threading::RWDataRaceCheck >
	{
		explicit shared_lock (const AE::Threading::RWDataRaceCheck &ref) :
			shared_lock< AE::Threading::RWDataRaceCheck >{ const_cast<AE::Threading::RWDataRaceCheck &>(ref) }
		{}
	};

}	// std

#else

namespace AE::Threading
{

	//
	// Data Race Check
	//
	struct DataRaceCheck
	{
		void lock () const		{}
		void unlock () const	{}
	};
	

	//
	// Read/Write Data Race Check
	//
	struct RWDataRaceCheck
	{
		void lock () const			{}
		void unlock () const		{}

		void lock_shared () const	{}
		void unlock_shared () const	{}
	};
	
}	// AE::Threading

#endif	// AE_ENABLE_DATA_RACE_CHECK

