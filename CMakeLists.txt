# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "AE"
		 VERSION 0.2.0
		 LANGUAGES C CXX )

message( STATUS "==========================================================================\n" )
message( STATUS "project '${PROJECT_NAME}' generation started" )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

#----------------------------------------------------------

set( AE_ENABLE_TESTS ON CACHE BOOL "enable tests" )
set( AE_ENABLE_SAMPLES ON CACHE BOOL "enable samples" )
set( AE_ENABLE_VTUNE_API OFF CACHE BOOL "enable helper functions for Intel VTune profiling" )
include( "cmake/config.cmake" )

#----------------------------------------------------------

set( CMAKE_DEBUG_POSTFIX "" )
set( CMAKE_PROFILE_POSTFIX "" )
set( CMAKE_DEVELOP_POSTFIX "" )
set( CMAKE_RELEASE_POSTFIX "" )
set( MAIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( AE_EXTERNAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE INTERNAL "" FORCE )

# this settings breaks native debugger on android
if (NOT DEFINED ANDROID)
	set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
endif ()

#----------------------------------------------------------

include( "cmake/external.cmake" )
add_subdirectory( "engine_tools/project_template" )
add_subdirectory( "engine" )

#----------------------------------------------------------

# crash reporting modules
add_subdirectory( "crash_report" )

#----------------------------------------------------------

# tools
if (${AE_DESKTOP})
	include( "engine_tools/res_pack/build.cmake" )
	add_subdirectory( "engine_tools" )
endif ()

#----------------------------------------------------------

# tests
if (${AE_ENABLE_TESTS})
	if (${AE_DESKTOP})
		enable_testing()
	endif ()
	add_subdirectory( "tests" )
endif ()

#----------------------------------------------------------

message( STATUS "project '${PROJECT_NAME}' generation ended" )
message( STATUS "\n==========================================================================" )
