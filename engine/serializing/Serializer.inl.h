// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

namespace AE::Serializing
{
	template <typename T>
	inline bool  Serializer::_SerializeObj (const T &obj)
	{
		if ( factory )
			return factory->Serialize( *this, obj );
		
		if constexpr( IsBaseOf< ISerializable, T >)
			return obj.Serialize( *this );
		else
		{
			ASSERT( !"unknown type" );
			return false;
		}
	}


	template <typename ...Args>
	inline bool  Serializer::operator () (const Args& ...args)
	{
		return _RecursiveSerialize( args... );
	}
	

	template <typename Arg0, typename ...Args>
	inline bool  Serializer::_RecursiveSerialize (const Arg0 &arg0, const Args& ...args)
	{
		bool	res = _Serialize( arg0 );

		if constexpr( CountOf<Args...>() > 0 )
			return res & _RecursiveSerialize( args... );
		else
			return res;
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const T &value)
	{
		if constexpr( IsPOD<T> )
			return stream->Write( value );
		else
			return _SerializeObj( value );
	}


	template <typename T>
	inline bool  Serializer::_Serialize (const TBytes<T> &value)
	{
		return _Serialize( T(value) );
	}


	template <typename F, typename S>
	inline bool  Serializer::_Serialize (const Pair<F,S> &value)
	{
		return _Serialize( value.first ) and _Serialize( value.second );
	}
	
	
	template <usize N>
	inline bool  Serializer::_Serialize (const BitSet<N> &value)
	{
		STATIC_ASSERT( N <= 64 );

		if constexpr( N <= 32 )
			return _Serialize( value.to_ulong() );
		else
			return _Serialize( value.to_ullong() );
	}


	template <typename T>
	inline bool  Serializer::_Serialize (ArrayView<T> arr)
	{
		bool	res = stream->Write( CheckCast<uint>(arr.size()) );
		
		if constexpr( IsPOD<T> )
			return res & stream->Write( arr.data(), SizeOf<T>*arr.size() );
		else
		{
			for (auto& item : arr) {
				res &= _Serialize( item );
			}
			return res;
		}
	}


	inline bool  Serializer::_Serialize (StringView str)
	{
		return	stream->Write( CheckCast<uint>(str.length()) ) and
				stream->Write( str );
	}
	

	template <typename T, uint I>
	inline bool  Serializer::_Serialize (const Vec<T,I> &vec)
	{
		return stream->Write( &vec.x, SizeOf<T>*I );
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const Rectangle<T> &rect)
	{
		return stream->Write( rect.data(), SizeOf<T>*4 );
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const RGBAColor<T> &col)
	{
		return stream->Write( col.data(), Bytes::SizeOf(col) );
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const HSVColor &col)
	{
		return stream->Write( col.data(), Bytes::SizeOf(col) );
	}
	

	template <usize Size, uint UID, uint Seed>
	inline bool  Serializer::_Serialize (const NamedID<Size, UID, true, Seed> &id)
	{
		return stream->Write( CheckCast<uint>(usize(id.GetHash())) );
	}
	

	template <usize Size, uint UID, uint Seed>
	inline bool  Serializer::_Serialize (const NamedID<Size, UID, false, Seed> &id)
	{
	#if AE_SERIALIZE_HASH_ONLY
		return stream->Write( CheckCast<uint>(usize(id.GetHash())) );
	#else
		return _Serialize( id.GetName() );
	#endif
	}


	template <typename K, typename V, typename H, typename E, typename A>
	inline bool  Serializer::_Serialize (const std::unordered_map<K,V,H,E,A> &map)
	{
		bool	res = stream->Write( CheckCast<uint>(map.size()) );

		for (auto iter = map.begin(); res & (iter != map.end()); ++iter)
		{
			res &= (_Serialize( iter->first ) and _Serialize( iter->second ));
		}
		return res;
	}


	template <typename T, typename H, typename E, typename A>
	inline bool  Serializer::_Serialize (const std::unordered_set<T,H,E,A> &set)
	{
		bool	res = stream->Write( CheckCast<uint>(set.size()) );

		for (auto iter = set.begin(); res & (iter != set.end()); ++iter)
		{
			res &= _Serialize( *iter );
		}
		return res;
	}
	
	
	template <typename K, typename V, usize S>
	inline bool  Serializer::_Serialize (const FixedMap<K,V,S> &map)
	{
		bool	res = stream->Write( CheckCast<uint>(map.size()) );

		for (usize i = 0; res & (i < map.size()); ++i)
		{
			res &= (_Serialize( map[i].first ) and _Serialize( map[i].second ));
		}
		return res;
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const Optional<T> &value)
	{
		bool	res = stream->Write( value.has_value() );

		if ( value )
			return res & _Serialize( *value );

		return res;
	}


	template <typename ...Types>
	inline bool  Serializer::_Serialize (const Union<Types...> &un)
	{
		return	stream->Write( CheckCast<uint>(un.index()) ) and
				_RecursiveSrializeUnion< Types... >( un );
	}
	

	template <typename T, typename ...Args, typename ...Types>
	inline bool  Serializer::_RecursiveSrializeUnion (const Union<Types...> &un)
	{
		if ( auto* value = UnionGetIf<T>( &un ))
			return _Serialize( *value );

		if constexpr( CountOf<Args...>() > 0 )
			return _RecursiveSrializeUnion< Args... >( un );
		else
			return false;
	}
	
}	// AE::Serializing
