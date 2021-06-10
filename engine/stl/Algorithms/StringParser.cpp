// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringParser.h"
#include "stl/Math/Math.h"

namespace AE::STL
{

/*
=================================================
	ToEndOfLine
=================================================
*/
	void StringParser::ToEndOfLine (StringView str, INOUT usize &pos)
	{
		if ( pos < str.length() and ((str[pos] == '\n') | (str[pos] == '\r')) )
			return;

		while ( pos < str.length() )
		{
			const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
				
			++pos;

			if ( (n == '\n') | (n == '\r') )
				return;
		}
	}
	
/*
=================================================
	ToBeginOfLine
=================================================
*/
	void StringParser::ToBeginOfLine (StringView str, INOUT usize &pos)
	{
		pos = Min( pos, str.length() );

		while ( pos <= str.length() )
		{
			const char	p = (pos-1) >= str.length() ? '\0' : str[pos-1];
				
			if ( (p == '\n') | (p == '\r') | (p == '\0') )
				return;

			--pos;
		}
		pos = 0;
	}
	
/*
=================================================
	IsBeginOfLine
=================================================
*/
	bool StringParser::IsBeginOfLine (StringView str, const usize pos)
	{
		usize	p = pos;
		ToBeginOfLine( str, INOUT p );
		return p == pos;
	}
	
/*
=================================================
	IsEndOfLine
=================================================
*/
	bool StringParser::IsEndOfLine (StringView str, const usize pos)
	{
		usize	p = pos;
		ToEndOfLine( str, INOUT p );
		return p == pos;
	}

/*
=================================================
	ToNextLine
=================================================
*/
	void StringParser::ToNextLine (StringView str, INOUT usize &pos)
	{
		while ( pos < str.length() )
		{
			const char	c = str[pos];
			const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
			
			++pos;

			// windows style "\r\n"
			if ( (c == '\r') & (n == '\n') )
			{
				++pos;
				return;
			}

			// linux style "\n" (or mac style "\r")
			if ( (c == '\n') | (c == '\r') )
				return;
		}
	}
	
/*
=================================================
	ToPrevLine
=================================================
*/
	void StringParser::ToPrevLine (StringView str, INOUT usize &pos)
	{
		pos = Min( pos, str.length() );

		while ( pos <= str.length() )
		{
			const char	c = str[pos];
			const char	p = (pos-1) >= str.length() ? 0 : str[pos-1];
			
			--pos;

			// windows style "\r\n"
			if ( (p == '\r') & (c == '\n') )
			{
				--pos;
				return;
			}

			// linux style "\n" (or mac style "\r")
			if ( (p == '\n') | (p == '\r') )
				return;
		}
	}
	
/*
=================================================
	CalculateNumberOfLines
=================================================
*/
	usize  StringParser::CalculateNumberOfLines (StringView str)
	{
		usize	lines = 0;

		for (usize pos = 0; pos < str.length(); ++lines)
		{
			ToNextLine( str, INOUT pos );
		}
		return lines;
	}
	
/*
=================================================
	MoveToLine
=================================================
*/
	bool StringParser::MoveToLine (StringView str, INOUT usize &pos, usize lineNumber)
	{
		usize	lines = 0;

		for (; (pos < str.length()) & (lines < lineNumber); ++lines)
		{
			ToNextLine( str, INOUT pos );
		}
		return lines == lineNumber;
	}

/*
=================================================
	ReadCurrLine
---
	Read line from begin of line to end of line
	and move to next line.
=================================================
*/
	void StringParser::ReadCurrLine (StringView str, INOUT usize &pos, OUT StringView &result)
	{
		ToBeginOfLine( str, INOUT pos );

		ReadLineToEnd( str, INOUT pos, OUT result );
	}
	
/*
=================================================
	ReadLineToEnd
----
	Read line from current position to end of line
	and move to next line.
=================================================
*/
	void StringParser::ReadLineToEnd (StringView str, INOUT usize &pos, OUT StringView &result)
	{
		const usize	prev_pos = pos;

		ToEndOfLine( str, INOUT pos );

		result = str.substr( prev_pos, pos - prev_pos );

		ToNextLine( str, INOUT pos );
	}

/*
=================================================
	ReadString
----
	read string from " to "
=================================================
*/
	bool StringParser::ReadString (StringView str, INOUT usize &pos, OUT StringView &result)
	{
		result = Default;

		for (; pos < str.length(); ++pos)
		{
			if ( str[pos] == '"' )
				break;
		}

		CHECK_ERR( str[pos] == '"' );

		const usize	begin = ++pos;

		for (; pos < str.length(); ++pos)
		{
			const char	c = str[pos];

			if ( c == '"' )
			{
				result = StringView{ str.data() + begin, pos - begin };
				++pos;
				return true;
			}
		}
	
		RETURN_ERR( "no pair for bracket \"" );
	}
	
/*
=================================================
	ReadTo
----
	read from 'pos' to symbol 'endSymbol'
=================================================
*/
	bool StringParser::ReadTo (StringView str, StringView endSymbol, INOUT usize &pos, OUT StringView &result)
	{
		result = Default;

		usize	start = pos;
		pos = str.find( endSymbol, start );

		if ( pos == StringView::npos )
			RETURN_ERR( "end symbol not found" );

		result = str.substr( start, pos );
		return true;
	}
	
/*
=================================================
	DivideLines
=================================================
*/
	void StringParser::DivideLines (StringView str, OUT Array<StringView> &lines)
	{
		lines.clear();

		usize	pos = 0;
		usize	prev = 0;

		while ( pos < str.length() )
		{
			ToEndOfLine( str, INOUT pos );

			if ( pos != prev ) {
				lines.push_back( str.substr( prev, pos-prev ));
			}

			ToNextLine( str, INOUT pos );

			prev = pos;
		}
	}
	
/*
=================================================
	Tokenize
=================================================
*/
	void StringParser::Tokenize (StringView str, const char divisor, OUT Array<StringView> &tokens)
	{
		usize	begin = 0;

		tokens.clear();

		for (usize i = 0; i < str.length(); ++i)
		{
			const char	c = str[i];

			if ( c == divisor )
			{
				tokens.push_back( StringView{ str.data() + begin, i - begin });
				begin = i+1;
			}
		}

		tokens.push_back( StringView{ str.data() + begin, str.length() - begin });
	}


}	// AE::STL
