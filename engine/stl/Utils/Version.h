// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/Cast.h"

namespace AE::STL
{

	//
	// 3 component Version
	//
	struct Version3
	{
		ushort	major	= 0;
		ushort	minor	= 0;
		uint	patch	= 0;

		Version3 () {}
		Version3 (uint maj, uint min, uint patch = 0) : major{CheckCast<ushort>(maj)}, minor{CheckCast<ushort>(min)}, patch{patch} {}

		ND_ bool  operator == (const Version3 &rhs) const;
		ND_ bool  operator >  (const Version3 &rhs) const;
		ND_ bool  operator >= (const Version3 &rhs) const;
	};
	


	//
	// 2 component Version
	//
	struct Version2
	{
		ushort	major	= 0;
		ushort	minor	= 0;

		Version2 () {}
		Version2 (uint maj, uint min) : major{CheckCast<ushort>(maj)}, minor{CheckCast<ushort>(min)} {}
		explicit Version2 (const Version3 &v) : major{v.major}, minor{v.minor} {}

		ND_ bool  operator == (const Version2 &rhs) const;
		ND_ bool  operator >  (const Version2 &rhs) const;
		ND_ bool  operator >= (const Version2 &rhs) const;
		
		ND_ bool  operator == (const Version3 &rhs) const	{ return *this == Version2{rhs}; }
		ND_ bool  operator >  (const Version3 &rhs) const	{ return *this >  Version2{rhs}; }
		ND_ bool  operator >= (const Version3 &rhs) const	{ return *this >= Version2{rhs}; }
	};
		



	inline bool  Version2::operator == (const Version2 &rhs) const
	{
		return	major == rhs.major and
				minor == rhs.minor;
	}

	inline bool  Version2::operator >  (const Version2 &rhs) const
	{
		return	major != rhs.major ? major > rhs.major :
									 minor > rhs.minor;
	}

	inline bool  Version2::operator >= (const Version2 &rhs) const
	{
		return not (rhs > *this);
	}
	

	inline bool  Version3::operator == (const Version3 &rhs) const
	{
		return	major == rhs.major and
				minor == rhs.minor and
				patch == rhs.patch;
	}

	inline bool  Version3::operator >  (const Version3 &rhs) const
	{
		return	major != rhs.major	? major > rhs.major :
				minor != rhs.minor	? minor > rhs.minor :
									  patch > rhs.patch;
	}

	inline bool  Version3::operator >= (const Version3 &rhs) const
	{
		return not (rhs > *this);
	}

}	// AE::STL
