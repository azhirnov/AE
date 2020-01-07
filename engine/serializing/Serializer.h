// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/Common.h"

namespace AE::Serializing
{

	//
	// Serializer
	//

	struct Serializer
	{
	// variables
	public:
		SharedPtr<WStream>			stream;
		Ptr<class ObjectFactory>	factory;


	// methods
	public:
		explicit Serializer () {}

		template <typename ...Args>
		bool operator () (const Args& ...args);

	private:
		template <typename Arg0, typename ...Args>
		bool _RecursiveSerialize (const Arg0 &arg0, const Args& ...args);

		template <typename T>				bool _SerializeObj (const T &);
		template <typename T>				bool _Serialize (const T &);
		template <typename T>				bool _Serialize (const Bytes<T> &);
		template <typename F, typename S>	bool _Serialize (const Pair<F,S> &);
		template <size_t N>					bool _Serialize (const BitSet<N> &);
		template <typename T>				bool _Serialize (ArrayView<T>);
		template <typename T, typename A>	bool _Serialize (const std::vector<T,A> &v)		{ return _Serialize(ArrayView<T>{v}); }
		template <typename T, size_t S>		bool _Serialize (const FixedArray<T,S> &arr)	{ return _Serialize(ArrayView<T>{arr}); }
		template <typename T, size_t S>		bool _Serialize (const StaticArray<T,S> &arr)	{ return _Serialize(ArrayView<T>{arr}); }
											bool _Serialize (StringView);
											bool _Serialize (const String &str)				{ return _Serialize(StringView{str}); }
		template <typename T, uint I>		bool _Serialize (const Vec<T,I> &);
		template <typename T>				bool _Serialize (const Rectangle<T> &);
		template <typename T>				bool _Serialize (const RGBAColor<T> &);
		template <typename T>				bool _Serialize (const HSVColor &);
		template <typename T, size_t S>		bool _Serialize (const TFixedString<T,S> &str)	{ return _Serialize(StringView{str}); }
		
		template <size_t Size, uint UID, uint Seed>
		bool _Serialize (const NamedID<Size, UID, true, Seed> &);

		template <size_t Size, uint UID, uint Seed>
		bool _Serialize (const NamedID<Size, UID, false, Seed> &);

		template <typename K, typename V, typename H, typename E, typename A>
		bool _Serialize (const std::unordered_map<K,V,H,E,A> &);

		template <typename T, typename H, typename E, typename A>
		bool _Serialize (const std::unordered_set<T,H,E,A> &);

		template <typename K, typename V, size_t S>
		bool _Serialize (const FixedMap<K,V,S> &);

		template <typename ...Types>
		bool _Serialize (const Union<Types...> &);

		template <typename T>
		bool _Serialize (const Optional<T> &);
		
		template <typename T, typename ...Args, typename ...Types>
		bool _RecursiveSrializeUnion (const Union<Types...> &);
	};

	

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
	inline bool  Serializer::_Serialize (const Bytes<T> &value)
	{
		return _Serialize( T(value) );
	}


	template <typename F, typename S>
	inline bool  Serializer::_Serialize (const Pair<F,S> &value)
	{
		return _Serialize( value.first ) and _Serialize( value.second );
	}
	
	
	template <size_t N>
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
		return stream->Write( col.data(), BytesU::SizeOf(col) );
	}
	

	template <typename T>
	inline bool  Serializer::_Serialize (const HSVColor &col)
	{
		return stream->Write( col.data(), BytesU::SizeOf(col) );
	}
	

	template <size_t Size, uint UID, uint Seed>
	inline bool  Serializer::_Serialize (const NamedID<Size, UID, true, Seed> &id)
	{
		return stream->Write( CheckCast<uint>(size_t(id.GetHash())) );
	}
	

	template <size_t Size, uint UID, uint Seed>
	inline bool  Serializer::_Serialize (const NamedID<Size, UID, false, Seed> &id)
	{
		return _Serialize( id.GetName() );
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
	
	
	template <typename K, typename V, size_t S>
	inline bool  Serializer::_Serialize (const FixedMap<K,V,S> &map)
	{
		bool	res = stream->Write( CheckCast<uint>(map.size()) );

		for (size_t i = 0; res & (i < map.size()); ++i)
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

		if constexpr( CountOf<Args...>() )
			return _RecursiveSrializeUnion< Args... >( un );
		else
			return false;
	}


}	// AE::Serializing
