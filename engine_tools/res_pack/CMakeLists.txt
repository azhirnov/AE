cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "ResPack" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if (DEFINED ANDROID)
	message( FATAL_ERROR "can't build for Android" )
endif ()

set( CMAKE_DEBUG_POSTFIX "" )
set( CMAKE_RELEASE_POSTFIX "" )
set( MAIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )

set( AE_ORIGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE INTERNAL "" FORCE )
set( AE_EXTERNAL_PATH "${AE_ORIGIN_PATH}/external" CACHE INTERNAL "" FORCE )

include( "${AE_ORIGIN_PATH}/cmake/external.cmake" )
add_subdirectory( "${AE_ORIGIN_PATH}/utils/project_template" "project_template" )
add_subdirectory( "${AE_ORIGIN_PATH}/engine/stl" "stl" )
add_subdirectory( "${AE_ORIGIN_PATH}/engine/scripting" "scripting" )
add_subdirectory( "${AE_ORIGIN_PATH}/engine/threading" "threading" )
add_subdirectory( "${AE_ORIGIN_PATH}/engine/serializing" "serializing" )
add_subdirectory( "sampler_packer" )

if (${AE_ENABLE_TESTS})
	add_subdirectory( "${AE_ORIGIN_PATH}/tests/asset_packer" "asset_packer_test" )
endif ()
