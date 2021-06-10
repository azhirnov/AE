# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

add_subdirectory( "stl" )
add_subdirectory( "scripting" )
add_subdirectory( "threading" )
add_subdirectory( "networking" )
add_subdirectory( "serializing" )
add_subdirectory( "platform" )
add_subdirectory( "graphics" )
#add_subdirectory( "ui" )
add_subdirectory( "ecs-st" )
#add_subdirectory( "ecs-mt" )
	
if (${AE_DESKTOP})
	add_subdirectory( "asset_packer" )
endif ()
	
# crash reporting modules
if (${AE_DESKTOP})
	add_subdirectory( "minidump_parser" )
	add_subdirectory( "crash_report_server" )
	add_subdirectory( "build_server" )
endif ()

if (DEFINED ANDROID)
	add_subdirectory( "test_launcher" )
endif ()
