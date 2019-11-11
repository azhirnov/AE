cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "AE" LANGUAGES C CXX )
message( STATUS "==========================================================================\n" )
message( STATUS "project '${PROJECT_NAME}' generation started" )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

#----------------------------------------------------------

set( AE_ENABLE_TESTS ON CACHE BOOL "enable tests" )
set( AE_ENABLE_SIMPLE_COMPILER_OPTIONS OFF CACHE BOOL "use simplified compiler settings if you have problem with it" )

set( AE_ENABLE_VTUNE_API OFF CACHE BOOL "enable helper functions for Intel VTune profiling" )

# External dependencies
set( AE_ENABLE_ANGELSCRIPT ON CACHE BOOL "enable AngelScript (for scripting)" )
set( AE_ENABLE_CURL ON CACHE BOOL "enable cURL (for networking)" )

#----------------------------------------------------------

set( CMAKE_DEBUG_POSTFIX "" )
set( CMAKE_RELEASE_POSTFIX "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )

include( "cmake/external.cmake" )
add_subdirectory( "utils/project_template" )
add_subdirectory( "engine/stl" )
add_subdirectory( "engine/scripting" )
add_subdirectory( "engine/threading" )
add_subdirectory( "engine/networking" )
#add_subdirectory( "engine/ecs" )

#----------------------------------------------------------

# tests
if (${AE_ENABLE_TESTS})
	enable_testing()
	add_subdirectory( "tests/stl" )
	add_subdirectory( "tests/scripting" )
	add_subdirectory( "tests/threading" )
	add_subdirectory( "tests/networking" )
	#add_subdirectory( "tests/ecs" )
endif()

#----------------------------------------------------------

message( STATUS "project '${PROJECT_NAME}' generation ended" )
message( STATUS "\n==========================================================================" )
