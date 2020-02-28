cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
if (DEFINED ANDROID)
	add_library( "Tests.Graphics" STATIC ${SOURCES} )
else()
	add_executable( "Tests.Graphics" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Graphics" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Graphics" PUBLIC "Graphics" )
target_compile_definitions( "Tests.Graphics" PUBLIC AE_TEST_GRAPHICS )

if (TARGET "ResourcePacker")
	add_dependencies( "Tests.Graphics" "ResourcePacker" )
endif ()

add_test( NAME "Tests.Graphics" COMMAND "Tests.Graphics" )
