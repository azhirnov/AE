cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.Threading" STATIC ${SOURCES} )
else()
	add_executable( "Tests.Threading" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Threading" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Threading" PUBLIC "Threading" )
target_compile_definitions( "Tests.Threading" PUBLIC AE_TEST_THREADING )
	
add_test( NAME "Tests.Threading" COMMAND "Tests.Threading" )
