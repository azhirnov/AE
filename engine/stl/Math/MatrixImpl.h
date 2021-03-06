// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Math
{

	//
	// Matrix
	//
	
	template <typename T>
	struct Matrix< T, Columns, Rows > final
	{
		STATIC_ASSERT( IsScalar<T> );
		STATIC_ASSERT( IsFloatPoint<T> );

	// types
	public:
		using Value_t		= T;
		using Self			= Matrix< T, Columns, Rows >;
		using _GLM_Mat_t	= glm::mat< glm::length_t(Columns), glm::length_t(Rows), T, GLMQuialifier >;
		using Col_t			= typename _GLM_Mat_t::col_type;
		using Row_t			= typename _GLM_Mat_t::row_type;
		

	// variables
	public:
		_GLM_Mat_t		_value;


	// methods
	public:
		GLM_CONSTEXPR Matrix () {}
		GLM_CONSTEXPR explicit Matrix (const _GLM_Mat_t &mat) : _value{mat} {}
		
		GLM_CONSTEXPR Matrix (const Self &other) : _value{other._value} {}
		Matrix (Self &&other) : _value{other._value} {}

		template <uint Columns2, uint Rows2>
		GLM_CONSTEXPR explicit Matrix (const Matrix<T, Columns2, Rows2> &other) : _value{other} {}

	#if Columns == 2
		GLM_CONSTEXPR Matrix (const Col_t &col0,
							  const Col_t &col1) :
			_value{ col0, col1 } {}
	#endif
		
	#if Columns == 3
		GLM_CONSTEXPR Matrix (const Col_t &col0,
							  const Col_t &col1,
							  const Col_t &col2) :
			_value{ col0, col1, col2 } {}

		explicit Matrix (const Quat<T> &q) : _value{glm::mat3_cast(q._value)} {}
	#endif
		
	#if Columns == 4
		GLM_CONSTEXPR Matrix (const Col_t &col0,
							  const Col_t &col1,
							  const Col_t &col2,
							  const Col_t &col3) :
			_value{ col0, col1, col2, col3 } {}

		explicit Matrix (const Quat<T> &q) : _value{glm::mat4_cast(q._value)} {}
	#endif
		
			Self&	Inverse ()								{ _value = glm::inverse( _value );  return *this; }
		ND_ Self	Inversed ()						const	{ return Self{ glm::inverse( _value )}; }

		ND_ static GLM_CONSTEXPR Self	Identity ()			{ return Self{ _GLM_Mat_t{ T(1) }}; }
		ND_ Matrix<T,Rows,Columns>		Transpose () const	{ return Matrix<T,Rows,Columns>{ glm::transpose( _value )}; }

		ND_ Self	operator + ()					const	{ return *this; }
		ND_ Self	operator - ()					const	{ return Self{ -_value }; }

			Self&	operator = (const Self &rhs)			{ _value = rhs._value;  return *this; }

			Self&	operator += (T rhs)				const	{ _value += rhs;  return *this; }
			Self&	operator -= (T rhs)				const	{ _value -= rhs;  return *this; }
			Self&	operator *= (T rhs)				const	{ _value *= rhs;  return *this; }
			Self&	operator /= (T rhs)				const	{ _value /= rhs;  return *this; }

		ND_ Self	operator +  (T rhs)				const	{ return Self{ _value + rhs }; }
		ND_ Self	operator -  (T rhs)				const	{ return Self{ _value - rhs }; }
		ND_ Self	operator *  (T rhs)				const	{ return Self{ _value * rhs }; }
		ND_ Self	operator /  (T rhs)				const	{ return Self{ _value / rhs }; }

		ND_ Col_t	operator *  (const Row_t &vec)	const	{ return _value * vec; }


		ND_ friend Row_t		operator * (const Col_t &lhs, const Self &rhs)	{ return lhs * rhs._value; }

		ND_ friend Self					operator * (T lhs, const Self &rhs)		{ return Self{ lhs * rhs._value }; }
		ND_ friend Self					operator / (T lhs, const Self &rhs)		{ return Self{ lhs / rhs._value }; }
		
		ND_ GLM_CONSTEXPR Col_t const&	operator [] (size_t i) const			{ return _value[ glm::length_t(i) ]; }
		ND_ GLM_CONSTEXPR Col_t&		operator [] (size_t i)					{ return _value[ glm::length_t(i) ]; }

		ND_ static GLM_CONSTEXPR size_t	size ()									{ return Columns; }
		ND_ static GLM_CONSTEXPR uint2	Dimension ()							{ return {Columns, Rows}; }

		
	#if Columns == 4
		ND_ static Self  Scale (T scale)										{ return Self{ glm::scale( Identity()._value, Vec<T,3>{ scale })}; }
		ND_ static Self  Scale (const Vec<T,3> &scale)							{ return Self{ glm::scale( Identity()._value, scale )}; }

		ND_ static Self  Translate (const Vec<T,3> &pos)						{ return Self{ glm::translate( Identity()._value, pos )}; }
		
		ND_ static Self  Ortho (const Rectangle<T> &viewport, const Vec<T,2> &range)			{ return Self{ glm::ortho( viewport.left, viewport.right, viewport.bottom, viewport.top, range[0], range[1] )}; }
		ND_ static Self  InfinitePerspective (RadiansTempl<T> fovY, T aspect, T zNear)			{ return Self{ glm::infinitePerspective( T(fovY), aspect, zNear )}; }
		ND_ static Self  Perspective (RadiansTempl<T> fovY, T aspect, const Vec<T,2> &range)	{ return Self{ glm::perspective( T(fovY), aspect, range[0], range[1] )}; }
		ND_ static Self  Perspective (RadiansTempl<T> fovY, const Vec<T,2> &viewport, const Vec<T,2> &range)	{ return Self{ glm::perspectiveFov( T(fovY), viewport.x, viewport.y, range[0], range[1] )}; }
		ND_ static Self  Frustum (const Rectangle<T> &viewport, const Vec<T,2> &range)			{ return Self{ glm::frustum( viewport.left, viewport.right, viewport.bottom, viewport.top, range[0], range[1] )}; }
	#endif	// Columns == 4
	};

}	// AE::Math
