# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'


set( AE_ENABLE_MEMLEAK_CHECKS OFF CACHE BOOL "enable memory leak checks" )

#----------------------------------------------------------
# advanced settings

set( AE_NO_EXCEPTIONS ON CACHE BOOL "disable C++ exceptions" )
set( AE_CI_BUILD OFF CACHE BOOL "CI settings" )
mark_as_advanced( AE_NO_EXCEPTIONS AE_CI_BUILD )

#----------------------------------------------------------
# internal constants

if (DEFINED ANDROID)
	set( AE_MOBILE  ON  CACHE INTERNAL "" FORCE )
	set( AE_DESKTOP OFF CACHE INTERNAL "" FORCE )
else ()
	set( AE_MOBILE  OFF CACHE INTERNAL "" FORCE )
	set( AE_DESKTOP ON  CACHE INTERNAL "" FORCE )
endif ()
