// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/Matrix.h"
#include "stl/Algorithms/ArrayUtils.h"

namespace AE::Math
{

	enum class EMatrixOrder
	{
		ColumnMajor,
		RowMajor,
	};


	template <typename T, uint Columns, uint Rows, EMatrixOrder Order, usize Align = alignof(T)>
	struct MatrixStorage;



	//
	// Column-major Matrix Storage
	//

	template <typename T, uint Columns, uint Rows, usize Align>
	struct MatrixStorage< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >
	{
	// types
	public:
		struct alignas(Align) _AlignedVec
		{
			T	data [Rows] = {};
		};

		using Self			= MatrixStorage< T, Columns, Rows, EMatrixOrder::ColumnMajor, Align >;
		using Transposed_t	= MatrixStorage< T, Rows, Columns, EMatrixOrder::ColumnMajor, Align >;
		using Column_t		= Vec< T, Rows >;
		using Row_t			= Vec< T, Columns >;


	// variables
	private:
		StaticArray< _AlignedVec, Columns >		_columns;
		
		//		  c0  c1  c2  c3
		//	r0	| 1 | 2 | 3 | X |	1 - left
		//	r1	|   |   |   | Y |	2 - up
		//	r2	|   |   |   | Z |	3 - forward
		//	r3	| 0 | 0 | 0 | W |


	// methods
	public:
		constexpr MatrixStorage () : _columns{} {}

		template <typename Arg0, typename ...Args>
		constexpr explicit MatrixStorage (const Arg0 &arg0, const Args& ...args)
		{
			if constexpr ( CountOf<Arg0, Args...>() == Columns * Rows )
				_CopyScalars<0>( arg0, args... );
			else
			if constexpr ( CountOf<Arg0, Args...>() == Columns )
				_CopyColumns<0>( arg0, args... );
			else
				STATIC_ASSERT(  (CountOf<Arg0, Args...>() == Columns * Rows) or
								(CountOf<Arg0, Args...>() == Columns) );
		}

		template <uint Columns2, uint Rows2, usize Align2>
		constexpr explicit MatrixStorage (const MatrixStorage< T, Columns2, Rows2, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				_columns[c].data[r] = ((c < Columns2) & (r < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
			}
		}
		
		template <uint Columns2, uint Rows2, usize Align2>
		constexpr explicit MatrixStorage (const MatrixStorage< T, Columns2, Rows2, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				_columns[c].data[r] = ((r < Columns2) & (c < Rows2)) ? other[r][c] : (c == r ? T(1) : T(0));
			}
		}

		template <uint Columns2, uint Rows2>
		explicit MatrixStorage (const Matrix< T, Columns2, Rows2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				_columns[c].data[r] = ((c < Columns2) & (r < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
			}
		}
		
		ND_ static constexpr Self  Identity ()
		{
			constexpr uint	cnt = Min(Columns, Rows);
			Self			result;

			for (uint i = 0; i < cnt; ++i) {
				result._columns[i].data[i] = T(1);
			}
			return result;
		}

		ND_ GLM_CONSTEXPR const Column_t  operator [] (uint index) const
		{
			auto&	d = _columns[index].data;

			if constexpr( Rows == 2 )
				return Column_t{ d[0], d[1] };
			
			if constexpr( Rows == 3 )
				return Column_t{ d[0], d[1], d[2] };
			
			if constexpr( Rows == 4 )
				return Column_t{ d[0], d[1], d[2], d[3] };
		}
		
		template <uint Columns2, uint Rows2>
		ND_ explicit operator Matrix< T, Columns2, Rows2 > () const
		{
			Matrix< T, Columns2, Rows2 >	result;
			for (uint c = 0; c < Columns2; ++c)
			for (uint r = 0; r < Rows2; ++r) {
				result[c][r] = ((c < Columns) & (r < Rows)) ? (*this)[c][r] : (c == r ? T(1) : T(0));
			}
			return result;
		}

		ND_ static constexpr usize		size ()							{ return Columns; }
		//ND_ static constexpr uint2	Dimension ()					{ return {Columns, Rows}; }

		ND_ static constexpr bool		IsColumnMajor ()				{ return true; }
		ND_ static constexpr bool		IsRowMajor ()					{ return not IsColumnMajor(); }


	private:
		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyScalars (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsScalar<Arg0> );
			_columns[I / Rows].data[I % Rows] = arg0;

			if constexpr ( I+1 < Columns * Rows )
				_CopyScalars< I+1 >( args... );
		}

		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyColumns (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsSameTypes< Arg0, Column_t > );
			std::memcpy( OUT _columns[I].data, &arg0.x, sizeof(T)*Rows );

			if constexpr ( I+1 < Columns )
				_CopyColumns< I+1 >( args... );
		}
	};



	//
	// Row-major Matrix Storage
	//

	template <typename T, uint Columns, uint Rows, usize Align>
	struct MatrixStorage< T, Columns, Rows, EMatrixOrder::RowMajor, Align >
	{
	// types
	public:
		struct alignas(Align) _AlignedVec
		{
			T	data [Columns] = {};
		};

		using Self			= MatrixStorage< T, Columns, Rows, EMatrixOrder::RowMajor, Align >;
		using Transposed_t	= MatrixStorage< T, Rows, Columns, EMatrixOrder::RowMajor, Align >;
		using Row_t			= Vec< T, Columns >;
		using Column_t		= Vec< T, Rows >;


	// variables
	private:
		StaticArray< _AlignedVec, Rows >	_rows;
		
		//		  c0  c1  c2  c3
		//	r0	| 1         | 0 |	1 - left
		//      |---------------|
		//	r1	| 2         | 0 |	2 - up
		//      |---------------|
		//	r2	| 3         | 0 |	3 - forward
		//      |---------------|
		//	r3	| X | Y | Z | W |
		

	// methods
	public:
		constexpr MatrixStorage () : _rows{} {}

		template <typename Arg0, typename ...Args>
		constexpr explicit MatrixStorage (const Arg0 &arg0, const Args& ...args)
		{
			if constexpr ( CountOf<Arg0, Args...>() == Columns * Rows )
				_CopyScalars<0>( arg0, args... );
			else
			if constexpr ( CountOf<Arg0, Args...>() == Rows )
				_CopyRows<0>( arg0, args... );
			else
				STATIC_ASSERT(  (CountOf<Arg0, Args...>() == Columns * Rows) or
								(CountOf<Arg0, Args...>() == Rows) );
		}
		
		template <uint Columns2, uint Rows2, usize Align2>
		constexpr explicit MatrixStorage (const MatrixStorage< T, Columns2, Rows2, EMatrixOrder::RowMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				_rows[r].data[c] = ((c < Columns2) & (r < Rows2)) ? other[r][c] : (c == r ? T(1) : T(0));
			}
		}
		
		template <uint Columns2, uint Rows2, usize Align2>
		constexpr explicit MatrixStorage (const MatrixStorage< T, Columns2, Rows2, EMatrixOrder::ColumnMajor, Align2 > &other)
		{
			for (uint r = 0; r < Rows; ++r)
			for (uint c = 0; c < Columns; ++c) {
				_rows[r].data[c] = ((r < Columns2) & (c < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
			}
		}
		
		template <uint Columns2, uint Rows2>
		explicit MatrixStorage (const Matrix< T, Columns2, Rows2 > &other)
		{
			for (uint c = 0; c < Columns; ++c)
			for (uint r = 0; r < Rows; ++r) {
				_rows[r].data[c] = ((c < Columns2) & (r < Rows2)) ? other[c][r] : (c == r ? T(1) : T(0));
			}
		}

		ND_ static Self  Identity ()
		{
			constexpr uint	cnt = Min(Columns, Rows);
			Self			result;

			for (uint i = 0; i < cnt; ++i) {
				result._rows[i].data[i] = T(1);
			}
			return result;
		}

		ND_ GLM_CONSTEXPR const Row_t  operator [] (uint index) const
		{
			auto&	d = _rows[index].data;
		
			if constexpr( Columns == 2 )
				return Row_t{ d[0], d[1] };
			
			if constexpr( Columns == 3 )
				return Row_t{ d[0], d[1], d[2] };
			
			if constexpr( Columns == 4 )
				return Row_t{ d[0], d[1], d[2], d[3] };
		}
		
		template <uint Columns2, uint Rows2>
		ND_ explicit operator Matrix< T, Columns2, Rows2 > () const
		{
			Matrix< T, Columns2, Rows2 >	result;
			for (uint c = 0; c < Columns2; ++c)
			for (uint r = 0; r < Rows2; ++r) {
				result[c][r] = ((c < Columns) & (r < Rows)) ? (*this)[r][c] : (c == r ? T(1) : T(0));
			}
			return result;
		}

		ND_ static constexpr usize		size ()							{ return Rows; }
		//ND_ static constexpr uint2	Dimension ()					{ return {Columns, Rows}; }
		
		ND_ static constexpr bool		IsColumnMajor ()				{ return false; }
		ND_ static constexpr bool		IsRowMajor ()					{ return not IsColumnMajor(); }


	private:
		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyScalars (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsScalar<Arg0> );
			_rows[I / Columns].data[I % Columns] = arg0;

			if constexpr ( I+1 < Columns * Rows )
				_CopyScalars< I+1 >( args... );
		}

		template <uint I, typename Arg0, typename ...Args>
		constexpr void _CopyRows (const Arg0 &arg0, const Args& ...args)
		{
			STATIC_ASSERT( IsSameTypes< Arg0, Row_t > );
			std::memcpy( OUT _rows[I].data, &arg0.x, sizeof(T)*Columns );

			if constexpr ( I+1 < Rows )
				_CopyRows< I+1 >( args... );
		}
	};


}	// AE::Math
