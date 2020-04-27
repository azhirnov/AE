// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_UTF8PROC

# include "stl/Common.h"
# include "utf8proc.h"

namespace AE::STL
{

/*
=================================================
	Utf8Decode
=================================================
*/
	forceinline char32_t  Utf8Decode (const char *str, size_t length, INOUT size_t &pos)
	{
		ASSERT( pos < length );

		utf8proc_int32_t	ref;
		utf8proc_ssize_t	res = utf8proc_iterate( Cast<utf8proc_uint8_t>(str + pos), length - pos, OUT &ref );
		
		ASSERT( res >= 0 );

		pos = res > 0 ? pos + res : pos;
		return ref;
	}

	forceinline char32_t  Utf8Decode (StringView str, INOUT size_t &pos)
	{
		return Utf8Decode( str.data(), str.length(), INOUT pos );
	}


}	// AE::STL

#endif	// AE_ENABLE_UTF8PROC
