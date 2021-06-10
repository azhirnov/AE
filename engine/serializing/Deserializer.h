// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/Serializer.h"

namespace AE::Serializing
{

	//
	// Deserializer
	//

	struct Deserializer
	{
	// variables
		RC<RStream>					stream;
		Ptr<class ObjectFactory>	factory;

	// methods
		explicit Deserializer () {}
		
		template <typename ...Args>
		bool  operator () (INOUT Args& ...args) const;
		bool  operator () (INOUT void *) const;
		
	private:
		template <typename Arg0, typename ...Args>
		bool  _RecursiveDeserialize (INOUT Arg0 &arg0, INOUT Args& ...args) const;

		template <typename T>				bool  _DeserializeObj (INOUT T &) const;
		template <typename T>				bool  _Deserialize (INOUT T &) const;
		template <typename T>				bool  _Deserialize (INOUT TBytes<T> &) const;
		template <typename F, typename S>	bool  _Deserialize (INOUT Pair<F,S> &) const;
		template <usize N>					bool  _Deserialize (INOUT BitSet<N> &) const;
											bool  _Deserialize (INOUT String &) const;
		template <typename T, usize S>		bool  _Deserialize (INOUT TFixedString<T,S> &) const;
		template <typename T, uint I>		bool  _Deserialize (INOUT Vec<T,I> &) const;
		template <typename T>				bool  _Deserialize (INOUT Rectangle<T> &) const;
		template <typename T>				bool  _Deserialize (INOUT RGBAColor<T> &) const;
		template <typename T>				bool  _Deserialize (INOUT HSVColor &) const;
		template <typename T, usize S>		bool  _Deserialize (INOUT StaticArray<T,S> &) const;
		template <typename T, usize S>		bool  _Deserialize (INOUT FixedArray<T,S> &) const;
		template <typename T, typename A>	bool  _Deserialize (INOUT std::vector<T,A> &) const;
		
		template <usize Size, uint UID, uint Seed>
		bool  _Deserialize (INOUT NamedID<Size, UID, true, Seed> &) const;
		
		template <usize Size, uint UID, uint Seed>
		bool  _Deserialize (INOUT NamedID<Size, UID, false, Seed> &) const;

		template <typename K, typename V, typename H, typename E, typename A>
		bool  _Deserialize (INOUT std::unordered_map<K,V,H,E,A> &) const;
		
		template <typename T, typename H, typename E, typename A>
		bool  _Deserialize (INOUT std::unordered_set<T,H,E,A> &) const;
		
		template <typename K, typename V, usize S>
		bool  _Deserialize (INOUT FixedMap<K,V,S> &) const;

		template <typename T>
		bool  _Deserialize (INOUT Optional<T> &) const;
	};

}	// AE::Serializing
