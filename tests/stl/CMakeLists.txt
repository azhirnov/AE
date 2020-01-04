cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.STL" STATIC ${SOURCES} )
else()
	add_executable( "Tests.STL" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.STL" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.STL" PUBLIC "STL" )
target_compile_definitions( "Tests.STL" PUBLIC AE_TEST_STL )
	
add_test( NAME "Tests.STL" COMMAND "Tests.STL" )
