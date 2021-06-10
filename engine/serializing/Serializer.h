// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

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
		RC<WStream>					stream;
		Ptr<class ObjectFactory>	factory;


	// methods
	public:
		explicit Serializer () {}

		template <typename ...Args>
		bool  operator () (const Args& ...args);

	private:
		template <typename Arg0, typename ...Args>
		bool  _RecursiveSerialize (const Arg0 &arg0, const Args& ...args);

		template <typename T>				bool  _SerializeObj (const T &);
		template <typename T>				bool  _Serialize (const T &);
		template <typename T>				bool  _Serialize (const TBytes<T> &);
		template <typename F, typename S>	bool  _Serialize (const Pair<F,S> &);
		template <usize N>					bool  _Serialize (const BitSet<N> &);
		template <typename T>				bool  _Serialize (ArrayView<T>);
		template <typename T, typename A>	bool  _Serialize (const std::vector<T,A> &v)		{ return _Serialize(ArrayView<T>{v}); }
		template <typename T, usize S>		bool  _Serialize (const FixedArray<T,S> &arr)	{ return _Serialize(ArrayView<T>{arr}); }
		template <typename T, usize S>		bool  _Serialize (const StaticArray<T,S> &arr)	{ return _Serialize(ArrayView<T>{arr}); }
											bool  _Serialize (StringView);
											bool  _Serialize (const String &str)				{ return _Serialize(StringView{str}); }
		template <typename T, uint I>		bool  _Serialize (const Vec<T,I> &);
		template <typename T>				bool  _Serialize (const Rectangle<T> &);
		template <typename T>				bool  _Serialize (const RGBAColor<T> &);
		template <typename T>				bool  _Serialize (const HSVColor &);
		template <typename T, usize S>		bool  _Serialize (const TFixedString<T,S> &str)	{ return _Serialize(StringView{str}); }
		
		template <usize Size, uint UID, uint Seed>
		bool  _Serialize (const NamedID<Size, UID, true, Seed> &);

		template <usize Size, uint UID, uint Seed>
		bool  _Serialize (const NamedID<Size, UID, false, Seed> &);

		template <typename K, typename V, typename H, typename E, typename A>
		bool  _Serialize (const std::unordered_map<K,V,H,E,A> &);

		template <typename T, typename H, typename E, typename A>
		bool  _Serialize (const std::unordered_set<T,H,E,A> &);

		template <typename K, typename V, usize S>
		bool  _Serialize (const FixedMap<K,V,S> &);

		template <typename ...Types>
		bool  _Serialize (const Union<Types...> &);

		template <typename T>
		bool  _Serialize (const Optional<T> &);
		
		template <typename T, typename ...Args, typename ...Types>
		bool  _RecursiveSrializeUnion (const Union<Types...> &);
	};

}	// AE::Serializing
