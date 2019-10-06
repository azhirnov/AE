cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

message( STATUS "==========================================================================\n" )
message( STATUS "project 'ECS-Engine' generation started" )

project( "ECS-Engine" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

#----------------------------------------------------------

set( FG_ENABLE_TESTS ON CACHE BOOL "enable tests" )
set( FG_ENABLE_SIMPLE_COMPILER_OPTIONS OFF CACHE BOOL "use simplified compiler settings if you have problem with it" )

#----------------------------------------------------------

set( CMAKE_DEBUG_POSTFIX "d" )
set( CMAKE_RELEASE_POSTFIX "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )

add_subdirectory( "utils/project_template" )
add_subdirectory( "stl" )
add_subdirectory( "tests/stl" )
