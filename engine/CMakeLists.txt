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
