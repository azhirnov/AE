# Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "ResPack" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if (${AE_MOBILE})
	message( FATAL_ERROR "can't build for Android" )
endif ()

if (NOT DEFINED AE_ORIGIN_SOURCE_PATH)
	message( STATUS "AE_ORIGIN_SOURCE_PATH is not defined" )
	set( AE_ORIGIN_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../.." )
endif ()
if (NOT DEFINED AE_TARGET_BINARY_PATH)
	message( STATUS "AE_TARGET_BINARY_PATH is not defined" )
	set( AE_TARGET_BINARY_PATH "${CMAKE_BINARY_DIR}/bin" )
endif ()

set( AE_EXTERNAL_PATH "${AE_ORIGIN_SOURCE_PATH}/external" CACHE INTERNAL "" FORCE )

set( CMAKE_DEBUG_POSTFIX "" )
set( CMAKE_RELEASE_POSTFIX "" )
set( MAIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE )
set( MAIN_BINARY_DIR "${AE_TARGET_BINARY_PATH}" CACHE INTERNAL "" FORCE )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )

include( "${AE_ORIGIN_SOURCE_PATH}/cmake/config.cmake" )
include( "${AE_ORIGIN_SOURCE_PATH}/cmake/external.cmake" )
add_subdirectory( "../project_template" "project_template" )
add_subdirectory( "${AE_ORIGIN_SOURCE_PATH}/engine/stl" "stl" )
target_compile_definitions( "STL" PUBLIC "AE_OPTIMIZE_IDS=0" )
add_subdirectory( "${AE_ORIGIN_SOURCE_PATH}/engine/scripting" "scripting" )
add_subdirectory( "${AE_ORIGIN_SOURCE_PATH}/engine/threading" "threading" )
add_subdirectory( "${AE_ORIGIN_SOURCE_PATH}/engine/serializing" "serializing" )
add_subdirectory( "pipeline_compiler" )
add_subdirectory( "sampler_packer" )
add_subdirectory( "asset_packer" )

if (${AE_ENABLE_TESTS})
	add_subdirectory( "${AE_ORIGIN_SOURCE_PATH}/tests/asset_packer" "asset_packer_test" )
endif ()
