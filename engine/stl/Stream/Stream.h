// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/MemUtils.h"
#include "stl/Utils/RefCounter.h"

namespace AE::STL
{

	//
	// Read-only Stream
	//
	
	class RStream : public EnableRC< RStream >
	{
	public:
		RStream () {}

		RStream (const RStream &) = delete;
		RStream (RStream &&) = delete;

		virtual ~RStream () {}

		RStream&  operator = (const RStream &) = delete;
		RStream&  operator = (RStream &&) = delete;

		ND_ virtual bool	IsOpen ()			const = 0;
		ND_ virtual Bytes	Position ()			const = 0;
		ND_ virtual Bytes	Size ()				const = 0;
		ND_ Bytes			RemainingSize ()	const { return Size() - Position(); }

			virtual bool	SeekSet (Bytes pos) = 0;
		ND_ virtual Bytes	Read2 (OUT void *buffer, Bytes size) = 0;
		

		bool  Read (OUT void *buffer, Bytes size)
		{
			return Read2( buffer, size ) == size;
		}
		

		template <typename T, typename A>
		bool  Read (usize length, OUT std::basic_string<T,A> &str)
		{
			str.resize( length );

			Bytes	expected_size	{ sizeof(str[0]) * str.length() };
			Bytes	current_size	= Read2( str.data(), expected_size );
		
			str.resize( usize(current_size / sizeof(str[0])) );

			return str.length() == length;
		}
		

		template <typename T, typename A>
		bool  Read (Bytes size, OUT std::basic_string<T,A> &str)
		{
			ASSERT( IsAligned( size, sizeof(T) ));
			return Read( usize(size) / sizeof(T), OUT str );
		}


		template <typename T, typename A>
		EnableIf<IsPOD<T>, bool>  Read (usize count, OUT std::vector<T,A> &arr)
		{
			arr.resize( count );

			Bytes	expected_size	{ sizeof(arr[0]) * arr.size() };
			Bytes	current_size	= Read2( arr.data(), expected_size );
		
			arr.resize( usize(current_size / sizeof(arr[0])) );

			return arr.size() == count;
		}
		

		template <typename T, typename A>
		EnableIf<IsPOD<T>, bool>  Read (Bytes size, OUT std::vector<T,A> &arr)
		{
			ASSERT( IsAligned( size, sizeof(T) ));
			return Read( usize(size) / sizeof(T), OUT arr );
		}


		template <typename T>
		EnableIf<IsPOD<T>, bool>  Read (OUT T &data)
		{
			return Read2( AddressOf(data), Bytes::SizeOf(data) ) == Bytes::SizeOf(data);
		}
	};



	//
	// Write-only Stream
	//
	
	class WStream : public EnableRC< WStream >
	{
	public:
		WStream () {}

		WStream (const WStream &) = delete;
		WStream (WStream &&) = delete;

		virtual ~WStream () {}

		WStream&  operator = (const WStream &) = delete;
		WStream&  operator = (WStream &&) = delete;

		ND_ virtual bool	IsOpen ()	const = 0;
		ND_ virtual Bytes	Position ()	const = 0;
		ND_ virtual Bytes	Size ()		const = 0;
		
			virtual bool	SeekSet (Bytes pos) = 0;
		ND_ virtual Bytes	Write2 (const void *buffer, Bytes size) = 0;
			virtual void	Flush () = 0;


		bool  Write (const void *buffer, Bytes size)
		{
			return Write2( buffer, size ) == size;
		}
		

		template <typename T>
		EnableIf<IsPOD<T>, bool>  Write (ArrayView<T> buf)
		{
			Bytes	size { sizeof(buf[0]) * buf.size() };

			return Write2( buf.data(), size ) == size;
		}
		
	
		template <typename T, typename A>
		bool  Write (const std::basic_string<T,A> str)
		{
			return Write( BasicStringView<T>{ str });
		}


		template <typename T>
		bool  Write (BasicStringView<T> str)
		{
			if ( str.empty() )
				return true;

			Bytes	size { sizeof(str[0]) * str.length() };

			return Write2( str.data(), size ) == size;
		}


		template <typename T>
		EnableIf<IsPOD<T>, bool>  Write (const T &data)
		{
			return Write2( AddressOf(data), Bytes::SizeOf(data) ) == Bytes::SizeOf(data);
		}
	};


}	// AE::STL

