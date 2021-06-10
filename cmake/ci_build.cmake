# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
#
# setup build on CI

if (${AE_CI_BUILD})
	message( STATUS "configured CI build" )
	
	set( AE_ENABLE_TESTS ON CACHE INTERNAL "" FORCE )
	set( AE_ENABLE_VTUNE_API OFF CACHE INTERNAL "" FORCE )

endif ()
