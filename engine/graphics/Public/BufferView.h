// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Buffer View
	//

	struct BufferView
	{
	// types
	public:
		struct Data
		{
			void *		ptr;
			Bytes		size;
		};

		struct ConstData
		{
			void const *	ptr;
			Bytes			size;
		};

		static constexpr uint	Count = 4;

		using Parts_t = FixedArray< Data, Count >;


	// variables
	private:
		Parts_t		_parts;


	// methods
	public:
		BufferView () {}
		BufferView (ArrayView<Data> parts) : _parts{parts} {}

		BufferView (const BufferView &) = delete;
		BufferView (BufferView &&) = default;

		BufferView&  operator = (const BufferView &) = delete;
		BufferView&  operator = (BufferView &&) = default;

		ND_ ArrayView<ConstData>	Parts () const	{ return ArrayView<ConstData>{ Cast<ConstData>(_parts.data()), _parts.size() }; }
		ND_ ArrayView<Data>			Parts ()		{ return _parts; }

		ND_ auto	begin ()			{ return Parts().begin(); }
		ND_ auto	end ()				{ return Parts().end(); }

		ND_ auto	begin ()	const	{ return Parts().begin(); }
		ND_ auto	end ()		const	{ return Parts().end(); }

		ND_ bool	empty ()	const	{ return _parts.empty(); }

			void	clear ()			{ _parts.clear(); }


		bool  try_push_back (void *ptr, Bytes size)
		{
			return _parts.try_push_back( Data{ ptr, size });
		}

		ND_ Bytes  DataSize () const
		{
			Bytes	result;
			for (auto& part : _parts) {
				result += part.size;
			}
			return result;
		}

		ND_ BufferView  section (Bytes offset, Bytes size)
		{
			BufferView	result;
			Bytes		off;

			for (auto& part : _parts)
			{
				if ( off >= offset )
					result.try_push_back( part.ptr + offset - off, Min( off + part.size, offset + size ));

				off += part.size;
			}
			return result;
		}
	};


}	// AE::Graphics
