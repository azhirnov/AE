cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.Platform" SHARED ${SOURCES} )
else()
	add_executable( "Tests.Platform" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Platform" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Platform" "Platform" )
set_target_properties( "Tests.Platform" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
	
add_test( NAME "Tests.Platform" COMMAND "Tests.Platform" )
