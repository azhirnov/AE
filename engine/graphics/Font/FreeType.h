// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_FREETYPE

# define _INC_STDDEF
# define _INC_STRING
# define _INC_STDLIB
# define _INC_ERRNO
# define _INC_STDIO

# include <ft2build.h>
# include <freetype/freetype.h>

# include FT_FREETYPE_H
# include FT_GLYPH_H
# include FT_BITMAP_H
# include FT_TRUETYPE_TABLES_H

#endif	// AE_ENABLE_FREETYPE
