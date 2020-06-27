# Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
#
# setup build on CI

if (${AE_CI_BUILD})
	message( STATUS "configured CI build" )
	
	set( AE_ENABLE_TESTS ON CACHE INTERNAL "" FORCE )
	set( AE_ENABLE_SIMPLE_COMPILER_OPTIONS OFF CACHE INTERNAL "" FORCE )
	set( AE_EXTERNALS_USE_STABLE_VERSIONS ON CACHE INTERNAL "" FORCE )
	set( AE_ENABLE_VTUNE_API OFF CACHE INTERNAL "" FORCE )

endif ()
