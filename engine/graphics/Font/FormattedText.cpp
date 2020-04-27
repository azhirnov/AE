// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Font/FormattedText.h"
#include "serializing/Serializer.h"
#include "serializing/Deserializer.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/Utf8.h"

namespace AE::Graphics
{
namespace {

	enum class EChunkType
	{
		Unknown,
		Initial,
		Bold,
		Italic,
		Underline,
		Strikeout,
		Style,
	};

	static constexpr uint	DefaultTextHeight = 16;
}

/*
=================================================
	Append
=================================================
*/
	FormattedText&  FormattedText::Append (const FormattedText &other)
	{
		Chunk*	last_chunk = null;
		for (last_chunk = _first; last_chunk; last_chunk = const_cast<Chunk*>(last_chunk->next))
		{}

		for (Chunk const* curr = other._first; curr; curr = curr->next)
		{
			size_t	size	= sizeof(Chunk) + curr->length;
			Chunk*	chunk	= Cast<Chunk>( _alloc.Alloc( BytesU{size}, BytesU{alignof(Chunk)} ));
			
			std::memcpy( OUT chunk, curr, size );
			
			if ( not _first )	_first = chunk;
			else				last_chunk->next = chunk;
			last_chunk = chunk;
		}

		return *this;
	}

/*
=================================================
	Append
=================================================
*/
	FormattedText&  FormattedText::Append (StringView str)
	{
		struct State
		{
			EChunkType	type		= Default;
			RGBA8u		color;
			uint		length		: 16;
			uint		height		: 9;
			uint		bold		: 1;
			uint		italic		: 1;
			uint		underline	: 1;
			uint		strikeout	: 1;

			State () : length{0}, height{0}, bold{0}, italic{0}, underline{0}, strikeout{0} {}
			State (const State &) = default;
		};
		//---------------------------------------------------------------------------

		static constexpr uint	tag_bold		= 'b' | (']' << 8);
		static constexpr uint	tag_italic		= 'i' | (']' << 8);
		static constexpr uint	tag_underline	= 'u' | (']' << 8);
		static constexpr uint	tag_strikeout	= 's' | (']' << 8);

		Array<State>	states;

		const auto	ParseOpeningTag = [&states, str] (INOUT size_t &pos) -> bool
		{
			const uint	u = *(str.data() + pos) | ((str.data() + pos)[1] << 8);
			switch ( u )
			{
				case tag_bold : {
					State	state = states.back();
					state.type = EChunkType::Bold;
					state.bold = true;
					states.push_back( state );
					pos += 2;
					return true;
				}
				case tag_italic : {
					State	state = states.back();
					state.type = EChunkType::Italic;
					state.italic = true;
					states.push_back( state );
					pos += 2;
					return true;
				}
				case tag_underline : {
					State	state = states.back();
					state.type = EChunkType::Underline;
					state.underline = true;
					states.push_back( state );
					pos += 2;
					return true;
				}
				case tag_strikeout : {
					State	state = states.back();
					state.type = EChunkType::Strikeout;
					state.strikeout = true;
					states.push_back( state );
					pos += 2;
					return true;
				}
			}

			// parse 'style ...'
			constexpr char	tag_style[] = "style";
			if ( memcmp( str.data() + pos, tag_style, sizeof(tag_style)-1 ) == 0 )
			{
				pos += sizeof(tag_style)-1;

				size_t	end		= Min( str.find( ']', pos ), str.length() );
				State	state	= states.back();
				state.type = EChunkType::Style;

				// search for 'size' and 'color' between 'start' and 'end'
				constexpr char	tag_size[]	= "size=";
				constexpr char	tag_color[]	= "color=#";

				for (; pos < end;)
				{
					const char	c = str[pos];
					if ( (c == ' ') | (c == '\t') )
					{
						++pos;
						continue;
					}

					// 'size=x'
					if ( memcmp( str.data()+pos, tag_size, sizeof(tag_size)-1 ) == 0 )
					{
						state.height = 0;
						for (pos += sizeof(tag_size)-1; pos < end;)
						{
							const char	k = str[pos];
							if ( (k >= '0') & (k <= '9') )
								state.height = state.height*10 + (k-'0');
							else
								break;
							++pos;
						}
						continue;
					}

					// 'color=#RRGGBBAA'
					if ( memcmp( str.data()+pos, tag_color, sizeof(tag_color)-1 ) == 0 )
					{
						uint	color = 0;
						for (pos += sizeof(tag_color)-1; pos < end;)
						{
							const char	k = str[pos];
							if ( (k >= '0') & (k <= '9') )
								color = (color << 4) + (k-'0');
							else if ( (k >= 'A') & (k <= 'F') )
								color = (color << 4) + (k-'A'+10);
							else if ( (k >= 'a') & (k <= 'f') )
								color = (color << 4) + (k-'a'+10);
							else
								break;
							++pos;
						}
						state.color = RGBA8u{ (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, (color & 0xFF) };
						continue;
					}

					ASSERT(false);
					break;
				}
				pos = end+1;
				states.push_back( state );
				return true;
			}
			return false;
		};

		const auto	ParseClosingTag = [&states, str] (INOUT size_t &pos) -> bool
		{
			if ( states.size() < 2 ) {
				ASSERT(false);
				return false;
			}
			const uint	u = *(str.data() + pos) | ((str.data() + pos)[1] << 8);
			switch ( u )
			{
				case tag_bold :
					if ( states.back().type == EChunkType::Bold ) {
						states.pop_back();
						pos += 2;
						return true;
					}
					return false;

				case tag_italic :
					if ( states.back().type == EChunkType::Italic ) {
						states.pop_back();
						pos += 2;
						return true;
					}
					return false;

				case tag_underline :
					if ( states.back().type == EChunkType::Underline ) {
						states.pop_back();
						pos += 2;
						return true;
					}
					return false;

				case tag_strikeout :
					if ( states.back().type == EChunkType::Strikeout ) {
						states.pop_back();
						pos += 2;
						return true;
					}
					return false;
			}

			// parse 'style'
			constexpr char	tag_style[] = "style]";
			if ( states.back().type == EChunkType::Style and
				 memcmp( str.data() + pos, tag_style, sizeof(tag_style)-1 ) == 0 )
			{
				states.pop_back();
				pos += sizeof(tag_style)-1;
				return true;
			}
			return false;
		};

		Chunk*	last_chunk = null;
		const auto	AddChunk = [this, &last_chunk] (const char* tag, const size_t length, const State &state)
		{
			if ( not length )
				return;

			void*	ptr		= _alloc.Alloc( BytesU{sizeof(Chunk) + length}, BytesU{alignof(Chunk)} );
			auto*	chunk	= PlacementNew<Chunk>( ptr );

			std::memcpy( OUT chunk->string, tag, length );
			chunk->string[length] = '\0';
			chunk->length		= length;
			chunk->color		= state.color;
			chunk->height		= state.height;
			chunk->bold			= state.bold;
			chunk->italic		= state.italic;
			chunk->underline	= state.underline;
			chunk->strikeout	= state.strikeout;
			
			if ( not _first )	_first = chunk;
			else				last_chunk->next = chunk;
			last_chunk = chunk;
		};
		//---------------------------------------------------------------------------



		// set default state
		{
			State&	def_state	= states.emplace_back();
			def_state.color		= HtmlColor::White;
			def_state.height	= DefaultTextHeight;
			def_state.type		= EChunkType::Initial;
		}

		size_t	start = 0, pos = 0;

		for (; pos < str.length();)
		{
			const size_t	old_pos = pos;

			// search pattern '[<key>...]' or '[/<key>]'
			const char32_t	c = Utf8Decode( str, INOUT pos );

			if ( c == '[' )
			{
				const size_t	tmp		= pos;
				const char32_t	n		= Utf8Decode( str, INOUT pos );
				const State		state	= states.back();
				bool			processed;

				if ( n == '/' )
					processed = ParseClosingTag( INOUT pos );
				else {
					pos = tmp;
					processed = ParseOpeningTag( INOUT pos );
				}

				if ( processed )
				{
					AddChunk( str.data() + start, old_pos - start, state );
					start = pos;
					continue;
				}
			}
		}

		AddChunk( str.data() + start, pos - start, states.back() );

		//ASSERT( states.size() == 1 and states.back().type == EChunkType::Initial );

		return *this;
	}
	
/*
=================================================
	ToString
=================================================
*/
	String  FormattedText::ToString () const
	{
		struct StackState
		{
			EChunkType		type;
			RGBA8u			color;
			uint			height;

			StackState () {}
			StackState (EChunkType type, RGBA8u color, uint height) : type{type}, color{color}, height{height} {}
		};

		String				result;
		Array<StackState>	stack;
		Chunk				state = {};

		const auto	CloseTag = [&] (Chunk const* chunk)
		{
			for (; stack.size();)
			{
				BEGIN_ENUM_CHECKS()
				switch ( stack.back().type )
				{
					case EChunkType::Bold :
						if ( state.bold & (state.bold != chunk->bold) ) {
							result << "[/b]";
							stack.pop_back();
							break;
						}
						return;

					case EChunkType::Italic :
						if ( state.italic & (state.italic != chunk->italic) ) {
							result << "[/i]";
							stack.pop_back();
							break;
						}
						return;

					case EChunkType::Underline :
						if ( state.underline & (state.underline != chunk->underline) ) {
							result << "[/u]";
							stack.pop_back();
							break;
						}
						return;

					case EChunkType::Strikeout :
						if ( state.strikeout & (state.strikeout != chunk->strikeout) ) {
							result << "[/s]";
							stack.pop_back();
							break;
						}
						return;

					case EChunkType::Style :
						if ( (state.color != chunk->color) | (state.height != chunk->height) ) {
							result << "[/style]";
							stack.pop_back();
							break;
						}
						return;

					case EChunkType::Unknown :
					case EChunkType::Initial :
					default :
						return;
				}
				END_ENUM_CHECKS();
			}
		};

		const auto	HexToString = [] (INOUT String &str, uint8_t x)
		{
			str << char((x >>  4) > 9 ? ('A' + ((x >>  4)-10)) : ('0' + (x >>  4)));
			str << char((x & 0xF) > 9 ? ('A' + ((x & 0xF)-10)) : ('0' + (x & 0xF)));
		};
		//---------------------------------------------------------------------------


		state.color  = HtmlColor::White;
		state.height = DefaultTextHeight;

		result.reserve( 1u << 10 );
		stack.emplace_back( EChunkType::Initial, state.color, uint(state.height) );

		for (auto* chunk = GetFirst(); chunk; chunk = chunk->next)
		{
			CloseTag( chunk );

			// open tag
			if ( chunk->bold & (state.bold != chunk->bold) ) {
				stack.emplace_back( EChunkType::Bold, chunk->color, chunk->height );
				result << "[b]";
			}

			if ( chunk->italic & (state.italic != chunk->italic) ) {
				stack.emplace_back( EChunkType::Italic, chunk->color, chunk->height );
				result << "[i]";
			}

			if ( chunk->underline & (state.underline != chunk->underline) ) {
				stack.emplace_back( EChunkType::Underline, chunk->color, chunk->height );
				result << "[u]";
			}

			if ( chunk->strikeout & (state.strikeout != chunk->strikeout) ) {
				stack.emplace_back( EChunkType::Strikeout, chunk->color, chunk->height );
				result << "[s]";
			}

			if ( (stack.back().color != chunk->color) | (stack.back().height != chunk->height) )
			{
				result << "[style";
				if ( stack.back().color != chunk->color ) {
					result << " color=#";
					HexToString( INOUT result, chunk->color.r );
					HexToString( INOUT result, chunk->color.g );
					HexToString( INOUT result, chunk->color.b );
					HexToString( INOUT result, chunk->color.a );
				}
				if ( stack.back().height != chunk->height )
					result << " size=" << STL::ToString( chunk->height );

				result << ']';
				stack.emplace_back( EChunkType::Style, chunk->color, chunk->height );
			}

			result << StringView{ chunk->string, chunk->length };

			state = *chunk;
		}

		ASSERT( stack.size() == 1 and stack.back().type == EChunkType::Initial );
		return result;
	}
	
/*
=================================================
	Clear
=================================================
*/
	void  FormattedText::Clear ()
	{
		_alloc.Release();
		_first = null;
	}

/*
=================================================
	Serialize
=================================================
*/
	bool  FormattedText::Serialize (Serializing::Serializer &ser) const
	{
		bool	result = true;
		uint	count  = 0;
		for (Chunk const* chunk = _first; chunk; chunk = chunk->next, ++count)
		{}

		result &= ser.stream->Write( count );
		for (Chunk const* chunk = _first; result & (chunk != null); chunk = chunk->next)
		{
			const uint	str_length = chunk->length;

			result &= ser.stream->Write( str_length );
			result &= ser.stream->Write( &chunk->color, BytesU{sizeof(Chunk) - offsetof(Chunk, color) + str_length} );
		}
		return result;
	}
	
/*
=================================================
	Deserialize
=================================================
*/
	bool  FormattedText::Deserialize (Serializing::Deserializer const &des)
	{
		bool	result = true;
		uint	count  = 0;
		
		_first = null;
		_alloc.Discard();

		result &= des.stream->Read( OUT count );

		for (uint i = 0; result & (i < count); ++i)
		{
			uint	str_length = 0;
			result &= des.stream->Read( OUT str_length );

			void*	ptr		= _alloc.Alloc( BytesU{sizeof(Chunk) + str_length}, BytesU{alignof(Chunk)} );
			Chunk*	chunk	= PlacementNew<Chunk>( ptr );

			result &= des.stream->Read( &chunk->color, BytesU{sizeof(Chunk) - offsetof(Chunk, color) + str_length} );
		}
		return result;
	}


}	// AE::Graphics
