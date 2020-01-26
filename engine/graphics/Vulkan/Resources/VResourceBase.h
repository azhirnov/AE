// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN

# include "graphics/Public/IDs.h"
# include "graphics/Vulkan/VCommon.h"

namespace AE::Graphics
{

	//
	// Resource Wrapper
	//

	template <typename ResType>
	class alignas(AE_CACHE_LINE) VResourceBase final
	{
	// types
	public:
		enum class EState : uint
		{
			Initial			= 0,
			Failed,
			Created,
		};

		using Self			= VResourceBase< ResType >;
		using Resource_t	= ResType;
		using Generation_t	= GfxResourceID::Generation_t;


	// variables
	private:
		// instance counter used to detect deprecated handles
		Atomic<uint>			_generation	= 0;

		Atomic<EState>			_state		= EState::Initial;

		ResType					_data;

		// reference counter may be used for cached resources like samples, pipeline layout and other
		mutable Atomic<int>		_refCounter	= 0;

		// cached resource may be deleted if reference counter is 1 and last usage was a long ago
		//mutable std::atomic<uint>	_lastUsage	= 0;


	// methods
	public:
		VResourceBase () {}

		VResourceBase (Self &&) = delete;
		VResourceBase (const Self &) = delete;

		Self& operator = (Self &&) = delete;
		Self& operator = (const Self &) = delete;

		~VResourceBase ()
		{
			ASSERT( IsDestroyed() );
		}

		void  AddRef () const
		{
			_refCounter.fetch_add( 1, EMemoryOrder::Relaxed );
		}

		ND_ int  ReleaseRef (int refCount) const
		{
			return (_refCounter.fetch_sub( refCount, EMemoryOrder::Relaxed ) - refCount);
		}
		

		ND_ bool			IsCreated ()		const	{ return _GetState() == EState::Created; }
		ND_ bool			IsDestroyed ()		const	{ return _GetState() <= EState::Failed; }

		ND_ Generation_t	GetGeneration ()	const	{ return Generation_t(_generation.load( EMemoryOrder::Relaxed )); }
		ND_ int				GetRefCount ()		const	{ return _refCounter.load( EMemoryOrder::Relaxed ); }
		//ND_ uint			GetLastUsage ()		const	{ return _lastUsage.load( EMemoryOrder::Relaxed ); }

		ND_ ResType&		Data ()						{ return _data; }
		ND_ ResType const&	Data ()				const	{ return _data; }


		ND_ bool  operator == (const Self &rhs) const	{ return _data == rhs._data; }
		ND_ bool  operator != (const Self &rhs) const	{ return not (_data == rhs._data); }


		template <typename ...Args>
		bool  Create (Args&& ...args)
		{
			ASSERT( IsDestroyed() );
			ASSERT( GetRefCount() == 0 );

			bool	result = _data.Create( std::forward<Args &&>( args )... );

			// set state and flush cache
			if ( result )
				_state.store( EState::Created, EMemoryOrder::Release );
			else
				_state.store( EState::Failed, EMemoryOrder::Release );

			return result;
		}

		template <typename ...Args>
		void  Destroy (Args&& ...args)
		{
			ASSERT( not IsDestroyed() );
			//ASSERT( GetRefCount() == 0 );

			_data.Destroy( std::forward<Args &&>( args )... );
			
			// flush cache
			ThreadFence( EMemoryOrder::Release );

			// update atomics
			_refCounter.store( 0, EMemoryOrder::Relaxed );
			_state.store( EState::Initial, EMemoryOrder::Relaxed );
			
			constexpr uint	max_gen = GfxResourceID::MaxGeneration();

			for (uint exp = _generation.load( EMemoryOrder::Relaxed);
				 not _generation.compare_exchange_weak( INOUT exp, (exp < max_gen ? exp + 1 : 0), EMemoryOrder::Relaxed );)
			{}
		}

	private:
		ND_ EState	_GetState ()	const	{ return _state.load( EMemoryOrder::Relaxed ); }
	};


}	// AE::Graphics


namespace std
{
	template <typename T>
	struct hash< AE::Graphics::VResourceBase<T> >
	{
		ND_ size_t  operator () (const AE::Graphics::VResourceBase<T> &value) const {
			return std::hash<T>{}( value.Data() );
		}
	};

}	// std

#endif	// AE_ENABLE_VULKAN
