// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#define PRIVATE_COMP_RETURN_ERR( _text_, _ret_ ) \
		{if ( not _quietWarnings ) { \
			AE_LOGE( _text_ ); \
		}else{ \
			AE_LOGI( _text_ ); \
		}return (_ret_); \
		}

#define COMP_RETURN_ERR( ... ) \
		PRIVATE_COMP_RETURN_ERR( AE_PRIVATE_GETARG_0( __VA_ARGS__ ), AE_PRIVATE_GETARG_1( __VA_ARGS__, ::AE::STL::Default ) )


#define PRIVATE_COMP_CHECK_ERR( _expr_, _ret_ ) \
		{if (( _expr_ )) {}\
		 else \
			PRIVATE_COMP_RETURN_ERR( AE_PRIVATE_TOSTRING(_expr_), (_ret_) ) \
		}

#define COMP_CHECK_ERR( ... ) \
		PRIVATE_COMP_CHECK_ERR( AE_PRIVATE_GETARG_0( __VA_ARGS__ ), AE_PRIVATE_GETARG_1( __VA_ARGS__, ::AE::STL::Default ) )

