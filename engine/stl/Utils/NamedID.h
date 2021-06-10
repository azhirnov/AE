// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/FixedString.h"
#include "stl/CompileTime/Hash.h"

namespace AE::STL
{

	//
	// ID With String
	//

	template <usize Size, uint UID, bool Optimize, uint Seed = UMax>
	struct NamedID
	{
	// types
	public:
		using Self	= NamedID< Size, UID, Optimize, Seed >;


	// variables
	private:
		HashVal						_hash;
		static constexpr HashVal	_emptyHash	= CT_Hash( "", 0, Seed );


	// methods
	public:
		constexpr NamedID () : _hash{_emptyHash} {}
		explicit constexpr NamedID (HashVal hash) : _hash{hash} {}
		explicit constexpr NamedID (StringView name)  : _hash{CT_Hash( name.data(), name.length(), Seed )} {}
		explicit constexpr NamedID (const char *name) : _hash{CT_Hash( name, UMax, Seed )} {}

		ND_ constexpr bool operator == (const Self &rhs) const		{ return _hash == rhs._hash; }
		ND_ constexpr bool operator != (const Self &rhs) const		{ return not (*this == rhs); }
		ND_ constexpr bool operator >  (const Self &rhs) const		{ return _hash > rhs._hash; }
		ND_ constexpr bool operator <  (const Self &rhs) const		{ return rhs > *this; }
		ND_ constexpr bool operator >= (const Self &rhs) const		{ return not (*this <  rhs); }
		ND_ constexpr bool operator <= (const Self &rhs) const		{ return not (*this >  rhs); }

		ND_ constexpr HashVal		GetHash ()		const			{ return _hash; }
		ND_ constexpr bool			IsDefined ()	const			{ return _hash != _emptyHash; }
		ND_ constexpr static bool	IsOptimized ()					{ return true; }
		ND_ constexpr static uint	GetSeed ()						{ return Seed; }
	};



	//
	// ID With String
	//

	template <usize Size, uint UID, uint Seed>
	struct NamedID< Size, UID, false, Seed >
	{
	// types
	public:
		using Self			= NamedID< Size, UID, false, Seed >;
		using Optimized_t	= NamedID< Size, UID, true, Seed >;


	// variables
	private:
		HashVal						_hash;
		FixedString<Size>			_name;
		static constexpr HashVal	_emptyHash	= CT_Hash( "", 0, Seed );


	// methods
	public:
		constexpr NamedID () : _hash{_emptyHash} {}
		explicit constexpr NamedID (HashVal hash) : _hash{hash} {}
		explicit constexpr NamedID (StringView name)  : _hash{CT_Hash( name.data(), name.length(), Seed )}, _name{name} {}
		explicit constexpr NamedID (const char *name) : _hash{CT_Hash( name, UMax, Seed )}, _name{name} {}

		template <usize StrSize>
		explicit constexpr NamedID (const FixedString<StrSize> &name) : _hash{CT_Hash( name.data(), name.length(), Seed )}, _name{name} {}

		ND_ constexpr bool operator == (const Self &rhs) const		{ return _hash == rhs._hash; }
		ND_ constexpr bool operator != (const Self &rhs) const		{ return not (*this == rhs); }
		ND_ constexpr bool operator >  (const Self &rhs) const		{ return _hash > rhs._hash; }
		ND_ constexpr bool operator <  (const Self &rhs) const		{ return rhs > *this; }
		ND_ constexpr bool operator >= (const Self &rhs) const		{ return not (*this <  rhs); }
		ND_ constexpr bool operator <= (const Self &rhs) const		{ return not (*this >  rhs); }

		ND_ constexpr operator Optimized_t ()			const		{ return Optimized_t{ _hash }; }

		ND_ constexpr StringView	GetName ()			const		{ return _name; }
		ND_ constexpr HashVal		GetHash ()			const		{ return _hash; }
		ND_ constexpr bool			IsDefined ()		const		{ return _hash != _emptyHash; }
		ND_ constexpr static bool	IsOptimized ()					{ return false; }
		ND_ constexpr static uint	GetSeed ()						{ return Seed; }
	};


}	// AE::STL

namespace std
{
	template <size_t Size, uint32_t UID, bool Optimize, uint32_t Seed>
	struct hash< AE::STL::NamedID<Size, UID, Optimize, Seed> >
	{
		ND_ size_t  operator () (const AE::STL::NamedID<Size, UID, Optimize, Seed> &value) const {
			return size_t(value.GetHash());
		}
	};

}	// std
