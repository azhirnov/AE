cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.Serializing" STATIC ${SOURCES} )
else()
	add_executable( "Tests.Serializing" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Serializing" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Serializing" PUBLIC "Serializing" )
target_compile_definitions( "Tests.Serializing" PUBLIC AE_TEST_SERIALIZING )
	
add_test( NAME "Tests.Serializing" COMMAND "Tests.Serializing" )
