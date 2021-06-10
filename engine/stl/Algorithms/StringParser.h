// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/StringUtils.h"

namespace AE::STL
{

	//
	// String Parser
	//

	struct StringParser final
	{
	public:
			static void  ToEndOfLine	(StringView str, INOUT usize &pos);
			static void  ToBeginOfLine	(StringView str, INOUT usize &pos);
			static void  ToNextLine		(StringView str, INOUT usize &pos);
			static void  ToPrevLine		(StringView str, INOUT usize &pos);

		ND_ static bool  IsBeginOfLine	(StringView str, usize pos);
		ND_ static bool  IsEndOfLine	(StringView str, usize pos);

		ND_ static usize CalculateNumberOfLines (StringView str);

			static bool  MoveToLine (StringView str, INOUT usize &pos, usize lineNumber);

			static void  ReadCurrLine (StringView str, INOUT usize &pos, OUT StringView &result);
			static void  ReadLineToEnd (StringView str, INOUT usize &pos, OUT StringView &result);

			static bool  ReadTo (StringView str, StringView endSymbol, INOUT usize &pos, OUT StringView &result);

			static bool  ReadString (StringView str, INOUT usize &pos, OUT StringView &result);
			
			static void  DivideLines (StringView str, OUT Array<StringView> &lines);

			static void  Tokenize (StringView str, const char divisor, OUT Array<StringView> &tokens);
	};


}	// AE::STL
