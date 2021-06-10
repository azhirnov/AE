// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Containers/ArrayView.h"

namespace AE::STL
{

	//
	// Structure View
	//

	template <typename T>
	struct StructView
	{
	// types
	public:
		using Self	= StructView< T >;

		struct const_iterator
		{
			Self const&		_ref;
			usize			_index = 0;

			const_iterator (const Self &ref, usize idx) : _ref{ref}, _index{idx} {}

			const_iterator&	operator ++ ()									{ ++_index;  return *this; }

			ND_ T const&	operator * ()							const	{ return _ref[_index]; }
			ND_ bool		operator == (const const_iterator &rhs)	const	{ return &_ref == &rhs._ref and _index == rhs._index; }
			ND_ bool		operator != (const const_iterator &rhs)	const	{ return not (*this == rhs); }
		};


	private:
		static constexpr uint	DBG_VIEW_COUNT = 400;

		struct _IViewer
		{
			virtual ~_IViewer () {}
		};
		

		template <typename St, usize Padding>
		struct _ViewerWithPaddingUnaligned final : _IViewer
		{
		// types
			#pragma pack (push, 1)
			struct Element {
				T			value;
				ubyte		_padding [Padding];
			};
			#pragma pack (pop)
			using ElementsPtr_t = Element const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerWithPaddingUnaligned (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) }
			{ STATIC_ASSERT( sizeof(Element) == sizeof(St) ); }
		};
		

		template <typename St, usize Padding>
		struct _ViewerWithPadding final : _IViewer
		{
		// types
			struct Element {
				T			value;
				ubyte		_padding [Padding];
			};
			using ElementsPtr_t = Element const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerWithPadding (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) }
			{ STATIC_ASSERT( sizeof(Element) == sizeof(St) ); }
		};


		template <typename St>
		struct _ViewerImpl final : _IViewer
		{
		// types
			using ElementsPtr_t = T const (*) [DBG_VIEW_COUNT];
			
		// variables
			ElementsPtr_t const		elements;
			
		// methods
			explicit _ViewerImpl (const void *ptr) : elements{ BitCast<ElementsPtr_t>(ptr) } {}
		};


	// variables
	private:
		void const *	_array		= null;
		usize			_count		= 0;
		uint			_stride		= 0;

		DEBUG_ONLY(
			UniquePtr<_IViewer>	_dbgView;	
		)


	// methods
	public:
		StructView ()
		{}

		StructView (ArrayView<T> arr) :
			_array{ arr.data() }, _count{ arr.size() }, _stride{ sizeof(T) }
		{
			DEBUG_ONLY( _dbgView = _CreateView<T, sizeof(T)>( _array ));
		}

		StructView (Self &&other) :
			_array{other._array}, _count{other._count}, _stride{other._stride}
		{
			DEBUG_ONLY( std::swap( _dbgView, other._dbgView ));
		}

		template <typename Class>
		StructView (ArrayView<Class> arr, T (Class::*member)) :
			_array{ arr.data() + OffsetOf(member) }, _count{ arr.size() }, _stride{ sizeof(Class) }
		{
			DEBUG_ONLY( _dbgView = _CreateView<Class, sizeof(Class)>( _array ));
		}
		
		StructView (const T* ptr, usize count) :
			_array{ ptr }, _count{ count }, _stride{ sizeof(T) }
		{
			DEBUG_ONLY( _dbgView = _CreateView<T, sizeof(T)>( _array ));
		}

		StructView (const void *ptr, usize count, uint stride) :
			_array{ptr}, _count{count}, _stride{stride}
		{}

		~StructView ()
		{}


		ND_ usize		size ()					const	{ return _count; }
		ND_ bool		empty ()				const	{ return _count == 0; }
		ND_ T const &	operator [] (usize i)	const	{ ASSERT(i < _count);  return *static_cast<T const *>(_array + Bytes(i * _stride)); }

		ND_ T const&	front ()				const	{ return operator[] (0); }
		ND_ T const&	back ()					const	{ return operator[] (_count-1); }

		ND_ const_iterator	begin ()			const	{ return const_iterator{ *this, 0 }; }
		ND_ const_iterator	end ()				const	{ return const_iterator{ *this, _count }; }


		ND_ bool  operator == (StructView<T> rhs) const
		{
			if ( (_array == rhs._array) & (_count == rhs._count) & (_stride == rhs._stride) )
				return true;

			if ( size() != rhs.size() )
				return false;

			for (usize i = 0; i < size(); ++i) {
				if ( not ((*this)[i] == rhs[i]) )
					return false;
			}
			return true;
		}


		ND_ StructView<T>  section (usize first, usize count) const
		{
			return first < size() ?
					StructView<T>{ _array + Bytes(first * _stride), Min(size() - first, count) } :
					StructView<T>{};
		}

		ND_ explicit operator Array<T> () const
		{
			Array<T>	result;
			result.resize( size() );

			for (usize i = 0; i < result.size(); ++i) {
				result[i] = (*this)[i];
			}
			return result;
		}


	private:
		template <typename Class, usize Stride>
		ND_ static UniquePtr<_IViewer>  _CreateView (const void *ptr)
		{
			STATIC_ASSERT( Stride >= sizeof(T) );
			const usize	padding = Stride - sizeof(T);

			if constexpr ( padding == 0 )
				return MakeUnique< _ViewerImpl< Class >>( ptr );
			else
			if constexpr ( padding % alignof(T) == 0 )
				return MakeUnique< _ViewerWithPadding< Class, padding >>( ptr );
			else
				return MakeUnique< _ViewerWithPaddingUnaligned< Class, padding >>( ptr );
		}
	};


}	// AE::STL
