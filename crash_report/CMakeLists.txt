# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

add_subdirectory( "crash_handler" )
if (${AE_DESKTOP})
	add_subdirectory( "build_server" )
	add_subdirectory( "minidump_parser" )
	add_subdirectory( "crash_report_sender" )
	add_subdirectory( "crash_report_server" )
endif ()
