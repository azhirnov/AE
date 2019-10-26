// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Defines.h"
#include "stl/Containers/StringView.h"

namespace AE::STL
{
	

	//
	// Logger
	//

	struct Logger
	{
		enum class EResult {
			Continue,
			Break,
			Abort,
		};
		
		static EResult  Info (const char *msg, const char *func, const char *file, int line);
		static EResult  Info (std::string_view msg, std::string_view func, std::string_view file, int line);

		static EResult  Error (const char *msg, const char *func, const char *file, int line);
		static EResult  Error (std::string_view msg, std::string_view func, std::string_view file, int line);
	};

}	// AE::STL


#define AE_PRIVATE_LOGX( _level_, _msg_, _file_, _line_ ) \
	BEGIN_ENUM_CHECKS() \
	{switch ( ::AE::STL::Logger::_level_( (_msg_), (AE_FUNCTION_NAME), (_file_), (_line_) ) ) \
	{ \
		case ::AE::STL::Logger::EResult::Continue :	break; \
		case ::AE::STL::Logger::EResult::Break :	AE_PRIVATE_BREAK_POINT();	break; \
		case ::AE::STL::Logger::EResult::Abort :	AE_PRIVATE_EXIT();			break; \
	}} \
	END_ENUM_CHECKS()

#define AE_PRIVATE_LOGI( _msg_, _file_, _line_ )	AE_PRIVATE_LOGX( Info, (_msg_), (_file_), (_line_) )
#define AE_PRIVATE_LOGE( _msg_, _file_, _line_ )	AE_PRIVATE_LOGX( Error, (_msg_), (_file_), (_line_) )
