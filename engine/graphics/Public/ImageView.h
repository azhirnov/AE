// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"
#include "graphics/Public/BufferView.h"
#include "graphics/Public/ResourceEnums.h"

namespace AE::Graphics
{

	//
	// Image View
	//

	struct ImageView
	{
	// types
	public:
		using Pixel_t	= BufferView::ConstData;
		using Row_t		= BufferView::Data;
		using CRow_t	= BufferView::ConstData;
		using Slice_t	= BufferView;

		using LoadRGBA32fFun_t	= void (*) (const CRow_t &, uint x, OUT RGBA32f &);
		using LoadRGBA32uFun_t	= void (*) (const CRow_t &, uint x, OUT RGBA32u &);
		using LoadRGBA32iFun_t	= void (*) (const CRow_t &, uint x, OUT RGBA32i &);


	// variables
	private:
		BufferView			_content;
		uint3				_dimension;
		Bytes				_rowPitch;
		Bytes				_slicePitch;
		uint				_bitsPerPixel	= 0;
		EPixelFormat		_format			= Default;
		LoadRGBA32fFun_t	_loadF4			= null;
		LoadRGBA32uFun_t	_loadU4			= null;
		LoadRGBA32iFun_t	_loadI4			= null;


	// methods
	public:
		ImageView () {}
		ImageView (BufferView&& content, const uint3 &dim, Bytes rowPitch, Bytes slicePitch, EPixelFormat format, EImageAspect aspect);

		ImageView (const ImageView &) = delete;
		ImageView (ImageView &&) = default;

		ImageView&  operator = (const ImageView &) = delete;
		ImageView&  operator = (ImageView &&) = default;

		ND_ uint3 const&	Dimension ()	const	{ return _dimension; }
		ND_ Bytes			RowPitch ()		const	{ return Bytes(_rowPitch); }
		ND_ Bytes			SlicePitch ()	const	{ return Bytes(_slicePitch); }
		ND_ uint			BitsPerPixel ()	const	{ return _bitsPerPixel; }
		ND_ Bytes			RowSize ()		const	{ return Bytes(_dimension.x * _bitsPerPixel) / 8; }
		ND_ Bytes			SliceSize ()	const	{ return Bytes(_rowPitch * _dimension.y); }
		ND_ EPixelFormat	Format ()		const	{ return _format; }
		ND_ auto			Parts ()				{ return _content.Parts(); }
		ND_ auto			Parts ()		const	{ return _content.Parts(); }

		
		ND_ Row_t  GetRow (uint y, uint z = 0)
		{
			ASSERT( y < _dimension.y );
			ASSERT( z < _dimension.z );

			const Bytes	row_size	= RowSize();
			const Bytes	row_offset	= _slicePitch * z + _rowPitch * y;
			Bytes			cur_offset;

			for (auto& part : _content.Parts())
			{
				if ( row_offset >= cur_offset and cur_offset < (row_offset + row_size) )
				{
					ASSERT( (cur_offset + part.size) >= (row_offset + row_size) );
					return Row_t{ part.ptr + (row_offset - cur_offset), row_size };
				}
				cur_offset += part.size;
			}

			ASSERT(false);
			return Row_t{};
		}
		
		ND_ CRow_t  GetRow (uint y, uint z = 0) const
		{
			auto	row = const_cast<ImageView*>(this)->GetRow( y, z );
			return CRow_t{ row.ptr, row.size };
		}

		
		ND_ Slice_t  GetSlice (uint z)
		{
			ASSERT( z < _dimension.z );
			
			const Bytes	slice_size		= SliceSize();
			const Bytes	slice_offset	= _slicePitch * z;
			Bytes			cur_offset;
			Slice_t			slice;
			
			for (auto& part : _content.Parts())
			{
				if ( cur_offset >= slice_offset and cur_offset < (slice_offset + slice_size) )
				{
					slice.try_push_back( part.ptr + (cur_offset - slice_offset),
										 Min( part.size + cur_offset, slice_offset + slice_size ) - cur_offset );
				}
				cur_offset += part.size;

				if ( cur_offset >= (slice_offset + slice_size) )
					break;
			}
			ASSERT( slice_size == slice.DataSize() );
			return slice;
		}
		
		ND_ Slice_t  GetSlice (uint z) const
		{
			return const_cast<ImageView*>(this)->GetSlice( z );
		}
		

		ND_ Pixel_t  GetPixel (const uint3 &point) const
		{
			ASSERT(All( point < _dimension ));

			CRow_t	row		= GetRow( point.y, point.z );
			Pixel_t	pixel	{ row.ptr + Bytes{(_bitsPerPixel * point.x) / 8}, Bytes{_bitsPerPixel / 8} };

			return pixel;
		}


		void Load (const uint3 &point, OUT RGBA32f &col) const
		{
			ASSERT( _loadF4 );
			_loadF4( GetRow( point.y, point.z ), point.x, OUT col );
		}

		void Load (const uint3 &point, OUT RGBA32u &col) const
		{
			ASSERT( _loadU4 );
			return _loadU4( GetRow( point.y, point.z ), point.x, OUT col );
		}

		void Load (const uint3 &point, OUT RGBA32i &col) const
		{
			ASSERT( _loadI4 );
			return _loadI4( GetRow( point.y, point.z ), point.x, OUT col );
		}
	};


}	// AE::Graphics
