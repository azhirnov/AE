// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringParser.h"
#include "UnitTest_Common.h"


namespace
{
	static void  StringParser_ToEndOfLine ()
	{
		usize	pos = 7;
		StringParser::ToEndOfLine( "1111\n2222\n3333333", pos );
		TEST( pos == 9 );

		pos = 0;
		StringParser::ToEndOfLine( "1111", pos );
		TEST( pos == 4 );
	
		pos = 0;
		StringParser::ToEndOfLine( "1111\r\n2222", pos );
		TEST( pos == 4 );
	
		pos = 0;
		StringParser::ToEndOfLine( "1111\r\n", pos );
		TEST( pos == 4 );
	}


	static void  StringParser_ToBeginOfLine ()
	{
		usize	pos = 7;
		StringParser::ToBeginOfLine( "1111\n2222\n3333333", pos );
		TEST( pos == 5 );
	
		pos = 6;
		StringParser::ToBeginOfLine( "11\r\n222\r\n33", pos );
		TEST( pos == 4 );
	}


	static void  StringParser_ToNextLine ()
	{
		usize	pos = 7;
		StringParser::ToNextLine( "1111\n2222\n3333333", pos );
		TEST( pos == 10 );
	
		pos = 7;
		StringParser::ToNextLine( "1111\n2222\r\n\r\n333", pos );
		TEST( pos == 11 );
	}


	static void  StringParser_ToPrevLine ()
	{
		usize	pos = 7;
		StringParser::ToPrevLine( "1111\n2222\n3333333", pos );
		TEST( pos == 4 );
	}


	static void  StringParser_ReadLine ()
	{
		StringView	str = "01234\r\n5678";
		StringView	line;
		usize		pos = 0;

		StringParser::ReadLineToEnd( str, pos, line );

		TEST( line == "01234" );
		TEST( pos == 7 );
	}


	static void  StringParser_ReadString ()
	{
		usize		pos = 0;
		StringView	result;
		StringParser::ReadString( "include \"123456\" ; ", pos, result );

		TEST( pos == 16 );
		TEST( result == "123456" );
	}


	static void  StringParser_CalculateNumberOfLines ()
	{
		usize	lines = StringParser::CalculateNumberOfLines( "1\n2\n3\r\n4\r\n5\n6\n7\r8\n9\n10" );
		TEST( lines == 10 );

		lines = StringParser::CalculateNumberOfLines( "1" );
		TEST( lines == 1 );

		lines = StringParser::CalculateNumberOfLines( "1\n2\n" );
		TEST( lines == 2 );
	}


	static void  StringParser_MoveToLine ()
	{
		usize	pos = 0;
		StringParser::MoveToLine( "1\n2\n3\r\n4\r\n5\n6\n7\r8\n9\n10", OUT pos, 0 );
		TEST( pos == 0 );
	
		pos = 0;
		StringParser::MoveToLine( "1\n2\n3\r\n4\r\n5\n6\n7\r8\n9\n10", OUT pos, 1 );
		TEST( pos == 2 );
	
		pos = 0;
		StringParser::MoveToLine( "1\n2\n3\r\n4\r\n5\n6\n7\r8\n9\n10", OUT pos, 9 );
		TEST( pos == 20 );
	}
}


extern void UnitTest_StringParser ()
{
	StringParser_ToEndOfLine();
	StringParser_ToBeginOfLine();
	StringParser_ToNextLine();
	StringParser_ToPrevLine();
	StringParser_ReadLine();
	StringParser_ReadString();
	StringParser_CalculateNumberOfLines();
	StringParser_MoveToLine();
	
	AE_LOGI( "UnitTest_StringParser - passed" );
}
