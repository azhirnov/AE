// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	[b][/b] - bold
	[i][/i] - italic
	[u][/u] - underline
	[s][/s] - strikethrough

	[style ...][/style]
		color=#00000000 - RRGGBBAA
		size=10 - size in dips
*/

#pragma once

#include "serializing/ISerializable.h"
#include "stl/Memory/LinearAllocator.h"
#include "graphics/Public/Common.h"

namespace AE::Graphics
{

	//
	// Formatted Text
	//

	class FormattedText final : public Serializing::ISerializable
	{
	// types
	public:
		struct Chunk
		{
			Chunk const*	next		= null;
			RGBA8u			color;
			uint			length		: 16;
			uint			height		: 9;
			uint			bold		: 1;
			uint			italic		: 1;
			uint			underline	: 1;
			uint			strikeout	: 1;
			char			string[1];		// null terminated utf8 string

			Chunk () = default;
		};

		using Self	= FormattedText;


	// variables
	private:
		LinearAllocator<>	_alloc;
		Chunk*				_first	= null;


	// methods
	public:
		FormattedText ()												{ _alloc.SetBlockSize( 512_b ); }
		FormattedText (StringView str) : FormattedText()				{ Append( str ); }
		FormattedText (const FormattedText &other) : FormattedText()	{ Append( other ); }
		FormattedText (FormattedText &&other) : _alloc{std::move(other._alloc)}, _first{other._first} { other._first = null; }

		Self&  Append (const FormattedText &);
		Self&  Append (StringView str);

		void   Clear ();

		FormattedText&  operator = (const FormattedText &rhs)	{ _alloc.Discard();  _first = null;  Append( rhs );  return *this; }
		FormattedText&  operator = (FormattedText &&rhs)		{ _alloc = std::move(rhs._alloc);  _first = rhs._first;  rhs._first = null;  return *this; }

		ND_ bool			Empty ()	const					{ return _first == null; }
		ND_ Chunk const*	GetFirst ()	const					{ return _first; }

		ND_ String			ToString ()	const;
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const &) override;
	};


}	// AE::Graphics
