// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "threading/Common.h"

#if AE_ENABLE_DATA_RACE_CHECK

#if defined(PLATFORM_ANDROID) or defined(AE_CI_BUILD)
#	define DRC_CHECK( ... )	CHECK_FATAL( __VA_ARGS__ )
#else
#	define DRC_CHECK( ... )	CHECK_ERR( __VA_ARGS__ )
#endif

namespace AE::Threading
{

	//
	// Data Race Check
	//

	struct DataRaceCheck
	{
	// variables
	private:
		mutable Atomic<usize>		_state  { 0 };


	// methods
	public:
		DataRaceCheck () {}

		ND_ bool  Lock () const
		{
			const usize	id		= usize(HashOf( std::this_thread::get_id() ));
			usize		curr	= _state.load( EMemoryOrder::Relaxed );
			
			if ( curr == id )
				return true;	// recursive lock

			curr	= 0;
			bool	locked	= _state.compare_exchange_strong( INOUT curr, id, EMemoryOrder::Relaxed );
			DRC_CHECK( curr == 0 );		// locked by another thread - race condition detected!
			DRC_CHECK( locked );

			return true;
		}

		void  Unlock () const
		{
			_state.store( 0, EMemoryOrder::Relaxed );
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
			DRC_CHECK( locked );	// locked by another thread - race condition detected!

			int		expected = _readCounter.load( EMemoryOrder::Relaxed );
			DRC_CHECK( expected <= 0 );	// has read lock(s) - race condition detected!

			// loop does not nedded here because mutex allow only single thread
			_readCounter.compare_exchange_strong( INOUT expected, expected-1, EMemoryOrder::Relaxed );
			DRC_CHECK( expected <= 0 );	// has read lock(s) - race condition detected!

			return true;
		}

		void  UnlockExclusive ()
		{
			_readCounter.fetch_add( 1, EMemoryOrder::Relaxed );
			_lockWrite.unlock();
		}


		ND_ bool  LockShared () const
		{
			int		expected	= 0;
			bool	locked		= false;
			do {
				locked = _readCounter.compare_exchange_weak( INOUT expected, expected+1, EMemoryOrder::Relaxed );
			
				// if has exclusive lock in current thread
				if ( expected < 0 and _lockWrite.try_lock() )
				{
					_lockWrite.unlock();
					return false;	// don't call 'UnlockShared'
				}
				DRC_CHECK( expected >= 0 );	// has write lock(s) - race condition detected!
			}
			while ( not locked );

			return true;
		}

		void  UnlockShared () const
		{
			_readCounter.fetch_sub( 1, EMemoryOrder::Relaxed );
		}
	};

}	// AE::Threading

#undef DRC_CHECK

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
