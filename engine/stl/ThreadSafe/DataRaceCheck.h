// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

#include <shared_mutex>

#ifdef AE_ENABLE_DATA_RACE_CHECK

#include <atomic>
#include <mutex>
#include <thread>

namespace AE::STL
{

	//
	// Data Race Check
	//

	struct DataRaceCheck
	{
	// variables
	private:
		mutable std::atomic<size_t>		_state  { 0 };


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
		mutable std::atomic<int>		_readCounter { 0 };


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

}	// AE::STL

namespace std
{
	template <>
	struct unique_lock< AE::STL::DataRaceCheck >
	{
	private:
		AE::STL::DataRaceCheck &	_lock;
		bool					_locked	= false;

	public:
		explicit unique_lock (AE::STL::DataRaceCheck &ref) : _lock{ref}
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
	struct unique_lock< const AE::STL::DataRaceCheck > :
		unique_lock< AE::STL::DataRaceCheck >
	{
		explicit unique_lock (const AE::STL::DataRaceCheck &ref) :
			unique_lock< AE::STL::DataRaceCheck >{ const_cast<AE::STL::DataRaceCheck &>(ref) }
		{}
	};


	template <>
	struct unique_lock< AE::STL::RWDataRaceCheck >
	{
	private:
		AE::STL::RWDataRaceCheck &	_lock;
		bool					_locked	= false;

	public:
		explicit unique_lock (AE::STL::RWDataRaceCheck &ref) : _lock{ref}
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
	struct unique_lock< const AE::STL::RWDataRaceCheck > :
		unique_lock< AE::STL::RWDataRaceCheck >
	{
		explicit unique_lock (const AE::STL::RWDataRaceCheck &ref) :
			unique_lock< AE::STL::RWDataRaceCheck >{ const_cast<AE::STL::RWDataRaceCheck &>(ref) }
		{}
	};


	template <>
	struct shared_lock< AE::STL::RWDataRaceCheck >
	{
	private:
		AE::STL::RWDataRaceCheck &	_lock;
		bool					_locked	= false;

	public:
		explicit shared_lock (AE::STL::RWDataRaceCheck &ref) : _lock{ref}
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
	struct shared_lock< const AE::STL::RWDataRaceCheck > :
		shared_lock< AE::STL::RWDataRaceCheck >
	{
		explicit shared_lock (const AE::STL::RWDataRaceCheck &ref) :
			shared_lock< AE::STL::RWDataRaceCheck >{ const_cast<AE::STL::RWDataRaceCheck &>(ref) }
		{}
	};

}	// std

namespace AE::STL
{

	//
	// Value with Data Race Check
	//

	template <typename T>
	struct DRCValue
	{
	// types
	private:
		struct Pointer
		{
		private:
			DRCValue<T> *	_ptr;
			
			Pointer (DRCValue<T> *ptr) : _ptr{ptr}		{}
			void _Lock ()								{ if ( _ptr ) _ptr->_drCheck.LockExclusive(); }
			void _Unlock ()								{ if ( _ptr ) _ptr->_drCheck.UnlockExclusive(); }

		public:
			Pointer (const Pointer &other)				{ _ptr = other._ptr;  _Lock(); }
			Pointer (Pointer &&other)					{ _ptr = other._ptr;  other._ptr = null; }
			~Pointer ()									{ _Unlock(); }

			Pointer&  operator = (const Pointer &rhs)	{ _Unlock();  _ptr = rhs._ptr;  _Lock();  return *this; }
			Pointer&  operator = (Pointer &&rhs)		{ _Unlock();  _ptr = rhs._ptr;  rhs._ptr = null;  return *this; }

			ND_ T *	operator -> ()						{ return &_ptr->_value; }
			ND_ T&	operator * ()						{ return _ptr->_value; }
		};
		

		struct CPointer
		{
		private:
			DRCValue<T> *	_ptr;
			
			CPointer (DRCValue<T> *ptr) : _ptr{ptr}		{}
			void _Lock ()								{ if ( _ptr ) _ptr->_drCheck.LockShared(); }
			void _Unlock ()								{ if ( _ptr ) _ptr->_drCheck.UnlockShared(); }

		public:
			CPointer (const CPointer &other)			{ _ptr = other._ptr;  _Lock(); }
			CPointer (CPointer &&other)					{ _ptr = other._ptr;  other._ptr = null; }
			~CPointer ()								{ _Unlock(); }

			CPointer&  operator = (const CPointer &rhs)	{ _Unlock();  _ptr = rhs._ptr;  _Lock();  return *this; }
			CPointer&  operator = (CPointer &&rhs)		{ _Unlock();  _ptr = rhs._ptr;  rhs._ptr = null;  return *this; }

			ND_ T const*  operator -> ()	const		{ return &_ptr->_value; }
			ND_ T const&  operator * ()		const		{ return _ptr->_value; }
		};


	// variables
	private:
		T					_value;
		RWDataRaceCheck		_drCheck;


	// methods
	public:
		DRCValue () {}
		DRCValue (const T &value) : _value{value}	{}
		DRCValue (T &&value) : _value{value}		{}
		~DRCValue ()								{ EXLOCK( _drCheck ); }

		DRCValue&  operator = (const T &rhs)		{ EXLOCK( _drCheck );  _value = rhs;  return *this; }
		DRCValue&  operator = (T &&rhs)				{ EXLOCK( _drCheck );  _value = std::move(rhs);  return *this; }

		DRCValue (const DRCValue &) = delete;
		DRCValue (DRCValue &&) = delete;
		DRCValue&  operator = (const DRCValue &) = delete;
		DRCValue&  operator = (DRCValue &&) = delete;

		ND_ Pointer   operator -> ()				{ return Pointer{ this }; }
		ND_ CPointer  operator -> ()		const	{ return CPointer{ this }; }
	};


}	// AE::STL


#else

namespace AE::STL
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
	

	//
	// Value with Data Race Check
	//
	template <typename T>
	struct DRCValue
	{
		T		_value;

		DRCValue () {}
		DRCValue (const T &val) : _value{val} {}
		DRCValue (T &&val) : _value{std::move(val)} {}

		ND_ T *			operator -> ()			{ return &_value; }
		ND_ T &			operator *  ()			{ return _value; }
		ND_ T const*	operator -> ()	const	{ return &_value; }
		ND_ T &			operator *  ()	const	{ return _value; }
	};


}	// AE::STL

#endif	// AE_ENABLE_DATA_RACE_CHECK

