// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

namespace AE::Serializing
{
	
	template <typename T>
	inline bool  Deserializer::_DeserializeObj (INOUT T &obj) const
	{
		if ( factory )
			return factory->Deserialize( *this, INOUT obj );
		
		if constexpr( IsBaseOf< ISerializable, T >)
			return obj.Deserialize( *this );
		else
		{
			ASSERT( !"unknown type" );
			return false;
		}
	}
	
	inline bool  Deserializer::operator () (INOUT void *obj) const
	{
		if ( factory )
			return factory->Deserialize( *this, INOUT obj );
		
		ASSERT( !"unknown type" );
		return false;
	}


	template <typename ...Args>
	inline bool  Deserializer::operator () (INOUT Args& ...args) const
	{
		return _RecursiveDeserialize( INOUT args... );
	}

		
	template <typename Arg0, typename ...Args>
	inline bool  Deserializer::_RecursiveDeserialize (INOUT Arg0 &arg0, INOUT Args& ...args) const
	{
		STATIC_ASSERT( not IsConst<Arg0> );

		bool res = _Deserialize( INOUT arg0 );

		if constexpr( CountOf<Args...>() > 0 )
			return res & _RecursiveDeserialize( INOUT args... );
		else
			return res;
	}
	

	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT T &value) const
	{
		if constexpr( IsPOD<T> )
			return stream->Read( OUT value );
		else
			return _DeserializeObj( INOUT value );
	}
	

	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT TBytes<T> &value) const
	{
		T		val	= {};
		bool	res = _Deserialize( OUT val );
		value = TBytes<T>{ val };
		return res;
	}


	template <typename F, typename S>
	inline bool  Deserializer::_Deserialize (INOUT Pair<F,S> &value) const
	{
		return _Deserialize( INOUT value.first ) and _Deserialize( INOUT value.second );
	}
	

	template <usize N>
	inline bool  Deserializer::_Deserialize (INOUT BitSet<N> &value) const
	{
		STATIC_ASSERT( N <= 64 );

		if constexpr( N <= 32 )
		{
			uint	bits = 0;
			bool	res  = _Deserialize( INOUT bits );
			value = BitSet<N>{ bits };
			return res;
		}
		else
		{
			ulong	bits = 0;
			bool		res  = _Deserialize( INOUT bits );
			value = BitSet<N>{ bits };
			return res;
		}
	}


	template <typename T, typename A>
	inline bool  Deserializer::_Deserialize (INOUT std::vector<T,A> &arr) const
	{
		uint	len = 0;
		if ( not stream->Read( OUT len ))
			return false;

		arr.resize( len );

		if constexpr( IsPOD<T> )
			return stream->Read( OUT arr.data(), SizeOf<T>*len );
		else
		{
			bool	res = true;
			for (auto& item : arr) {
				res &= _Deserialize( INOUT item );
			}
			return res;
		}
	}


	template <typename T, usize S>
	inline bool  Deserializer::_Deserialize (INOUT StaticArray<T,S> &arr) const
	{
		uint	len = 0;
		if ( not stream->Read( OUT len ) or (len > S) )
			return false;

		if constexpr( IsPOD<T> )
			return stream->Read( OUT arr.data(), SizeOf<T>*arr.size() );
		else
		{
			bool	res = true;
			for (auto& item : arr) {
				res &= _Deserialize( INOUT item );
			}
			return res;
		}
	}


	template <typename T, usize S>
	inline bool  Deserializer::_Deserialize (INOUT FixedArray<T,S> &arr) const
	{
		uint	len = 0;
		if ( not stream->Read( OUT len ) or (len > S) )
			return false;

		arr.resize( len );

		if constexpr( IsPOD<T> )
			return stream->Read( OUT arr.data(), SizeOf<T>*arr.size() );
		else
		{
			bool	res = true;
			for (auto& item : arr) {
				res &= _Deserialize( INOUT item );
			}
			return res;
		}
	}


	inline bool  Deserializer::_Deserialize (INOUT String &str) const
	{
		uint	len = 0;
		return	stream->Read( OUT len ) and
				stream->Read( len, OUT str );
	}
	

	template <typename T, usize S>
	inline bool  Deserializer::_Deserialize (INOUT TFixedString<T,S> &str) const
	{
		uint	len = 0;
		if ( not stream->Read( OUT len ))
			return false;

		str.resize( len );
		return stream->Read( OUT str.data(), SizeOf<T>*len );
	}


	template <typename T, uint I>
	inline bool  Deserializer::_Deserialize (INOUT Vec<T,I> &vec) const
	{
		return stream->Read( OUT &vec.x, SizeOf<T>*I );
	}
	

	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT Rectangle<T> &rect) const
	{
		return stream->Read( OUT rect.data(), SizeOf<T>*4 );
	}
	

	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT RGBAColor<T> &col) const
	{
		return stream->Read( OUT col.data(), Bytes::SizeOf(col) );
	}


	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT HSVColor &col) const
	{
		return stream->Read( OUT col.data(), Bytes::SizeOf(col) );
	}
	

	template <usize Size, uint UID, uint Seed>
	inline bool  Deserializer::_Deserialize (INOUT NamedID<Size, UID, true, Seed> &id) const
	{
		uint	hash = 0;
		bool	res  = stream->Read( OUT hash );

		id = NamedID<Size, UID, true, Seed>{ HashVal{ hash }};
		return res;
	}
	

	template <usize Size, uint UID, uint Seed>
	inline bool  Deserializer::_Deserialize (INOUT NamedID<Size, UID, false, Seed> &id) const
	{
	#if AE_SERIALIZE_HASH_ONLY
		uint	hash = 0;
		bool	res  = stream->Read( OUT hash );

		id = NamedID<Size, UID, false, Seed>{ HashVal{ hash }};
		return res;

	#else
		FixedString<Size>	str;
		if ( not _Deserialize( str ))
			return false;

		id = NamedID<Size, UID, false, Seed>{ str };
		return true;
	#endif
	}
	

	template <typename K, typename V, typename H, typename E, typename A>
	inline bool  Deserializer::_Deserialize (INOUT std::unordered_map<K,V,H,E,A> &map) const
	{
		uint	count	= 0;
		bool	res		= stream->Read( OUT count );

		for (uint i = 0; res & (i < count); ++i)
		{
			K	key		= {};
			V	value	= {};
			res &= (_Deserialize( OUT key ) and _Deserialize( OUT value ));
			res &= map.insert_or_assign( RVRef(key), RVRef(value) ).second;
		}
		return res;
	}
	

	template <typename T, typename H, typename E, typename A>
	inline bool  Deserializer::_Deserialize (INOUT std::unordered_set<T,H,E,A> &set) const
	{
		uint	count	= 0;
		bool	res		= stream->Read( OUT count );

		for (uint i = 0; res & (i < count); ++i)
		{
			T	value = {};
			res &= _Deserialize( OUT value );
			res &= set.insert( RVRef(value) );
		}
		return res;
	}
	

	template <typename K, typename V, usize S>
	inline bool  Deserializer::_Deserialize (INOUT FixedMap<K,V,S> &map) const
	{
		uint	count	= 0;
		bool	res		= stream->Read( OUT count );

		if ( not res or (count > S) )
			return false;

		for (uint i = 0; res & (i < count); ++i)
		{
			K	key		= {};
			V	value	= {};
			res &= (_Deserialize( OUT key ) and _Deserialize( OUT value ));
			res &= map.insert_or_assign( RVRef(key), RVRef(value) ).second;
		}
		return res;
	}
	

	template <typename T>
	inline bool  Deserializer::_Deserialize (INOUT Optional<T> &value) const
	{
		bool	has_value;
		bool	res			= stream->Read( OUT has_value );

		if ( res & has_value )
			return _Deserialize( INOUT value.emplace() );
		
		return res;
	}
	
}	// AE::Serializing
