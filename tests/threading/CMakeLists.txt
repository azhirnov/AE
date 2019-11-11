cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.Threading" SHARED ${SOURCES} )
else()
	add_executable( "Tests.Threading" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Threading" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Threading" "Threading" )
set_target_properties( "Tests.Threading" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
	
add_test( NAME "Tests.Threading" COMMAND "Tests.Threading" )
