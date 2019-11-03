// from https://github.com/DaemonSnake/unconstexpr (MIT license)

#pragma once

#include "stl/Common.h"

namespace AE::STL
{

	//
	// Compile Time Unique Value
	//

	class CT_UniqueValue
	{
	private:
		struct Impl
		{
			template <uint N>
			struct Flag
			{
				friend constexpr bool adl_flag (Flag<N>);
			};
		
			template <uint N>
			struct Writer
			{
				friend constexpr bool adl_flag (Flag<N>) { return true; }
				static constexpr uint value = N;
			};

			template <uint N = 0, bool = adl_flag(Flag<N>{})>
			static constexpr uint  Reader (int, uint r = Reader<N+1>(0))
			{
				return r;
			}

			template <uint N = 0>
			static constexpr uint  Reader (float)
			{
				return Writer<N>::value;
			}
		};

	public:
		template <uint N = Impl::Reader(0)>
		static constexpr uint value = N;
	};



	//
	// Compile Time Counter
	//

	namespace _ae_stl_hidden_
	{
		template <typename Tag = void, typename Type = uint>
		class CT_Counter_Impl
		{
		private:
			template <Type N>
			struct Flag
			{
				friend constexpr bool adl_flag (Flag<N>);
			};

			template <Type N>
			struct Writer
			{
				friend constexpr bool adl_flag (Flag<N>) { return true; }
				static constexpr Type value = N;
			};

			template <Type N, bool = adl_flag(Flag<N>{})>
			static constexpr Type  Reader (int, Flag<N>, Type r = Reader(0, Flag<N + 1>{}))
			{
				return r;
			}

			template <Type N>
			static constexpr Type  Reader (float, Flag<N>)
			{
				return N;
			}

		public:
			static constexpr Type  Value (Type r = Reader(0, Flag<0>{}))
			{
				return r;
			}

			template <Type ValueType = Value()>
			static constexpr Type  Next (Type r = Writer<ValueType>::value)
			{
				return r + 1;
			}
		};
		
		template <uint N>
		class CT_UniqueType_Impl {};

	}	// _ae_stl_hidden_

	
	template <typename Type = uint, uint I = CT_UniqueValue::value<>>
	using CT_Counter = _ae_stl_hidden_::CT_Counter_Impl< _ae_stl_hidden_::CT_UniqueType_Impl<I>, Type >;
	
	

	//
	// Compile Time Unique Type
	//

	template <uint I = CT_UniqueValue::value<>>
	using CT_UniqueType = _ae_stl_hidden_::CT_UniqueType_Impl<I>;


}	// AE::STL
