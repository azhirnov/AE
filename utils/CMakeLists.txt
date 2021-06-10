# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

if (${AE_DESKTOP})
	add_subdirectory( "algorithms" )
endif ()
