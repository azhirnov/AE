// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/MemUtils.h"

namespace AE::STL
{

	//
	// Read-only Stream
	//
	
	class RStream : public std::enable_shared_from_this< RStream >
	{
	public:
		RStream () {}

		RStream (const RStream &) = delete;
		RStream (RStream &&) = delete;

		virtual ~RStream () {}

		RStream&  operator = (const RStream &) = delete;
		RStream&  operator = (RStream &&) = delete;

		ND_ virtual bool	IsOpen ()			const = 0;
		ND_ virtual BytesU	Position ()			const = 0;
		ND_ virtual BytesU	Size ()				const = 0;
		ND_ BytesU			RemainingSize ()	const { return Size() - Position(); }

			virtual bool	SeekSet (BytesU pos) = 0;
		ND_ virtual BytesU	Read2 (OUT void *buffer, BytesU size) = 0;
		

		bool  Read (OUT void *buffer, BytesU size)
		{
			return Read2( buffer, size ) == size;
		}
		

		template <typename T, typename A>
		bool  Read (size_t length, OUT std::basic_string<T,A> &str)
		{
			str.resize( length );

			BytesU	expected_size	{ sizeof(str[0]) * str.length() };
			BytesU	current_size	= Read2( str.data(), expected_size );
		
			str.resize( size_t(current_size / sizeof(str[0])) );

			return str.length() == length;
		}
		

		template <typename T, typename A>
		EnableIf<IsPOD<T>, bool>  Read (size_t count, OUT std::vector<T,A> &arr)
		{
			arr.resize( count );

			BytesU	expected_size	{ sizeof(arr[0]) * arr.size() };
			BytesU	current_size	= Read2( arr.data(), expected_size );
		
			arr.resize( size_t(current_size / sizeof(arr[0])) );

			return arr.size() == count;
		}


		template <typename T>
		EnableIf<IsPOD<T>, bool>  Read (OUT T &data)
		{
			return Read2( AddressOf(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}
	};



	//
	// Write-only Stream
	//
	
	class WStream : public std::enable_shared_from_this< WStream >
	{
	public:
		WStream () {}

		WStream (const WStream &) = delete;
		WStream (WStream &&) = delete;

		virtual ~WStream () {}

		WStream&  operator = (const WStream &) = delete;
		WStream&  operator = (WStream &&) = delete;

		ND_ virtual bool	IsOpen ()	const = 0;
		ND_ virtual BytesU	Position ()	const = 0;
		ND_ virtual BytesU	Size ()		const = 0;
		
			virtual bool	SeekSet (BytesU pos) = 0;
		ND_ virtual BytesU	Write2 (const void *buffer, BytesU size) = 0;
			virtual void	Flush () = 0;


		bool  Write (const void *buffer, BytesU size)
		{
			return Write2( buffer, size ) == size;
		}
		

		template <typename T>
		EnableIf<IsPOD<T>, bool>  Write (ArrayView<T> buf)
		{
			BytesU	size { sizeof(buf[0]) * buf.size() };

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

			BytesU	size { sizeof(str[0]) * str.length() };

			return Write2( str.data(), size ) == size;
		}


		template <typename T>
		EnableIf<IsPOD<T>, bool>  Write (const T &data)
		{
			return Write2( AddressOf(data), BytesU::SizeOf(data) ) == BytesU::SizeOf(data);
		}
	};


}	// AE::STL

