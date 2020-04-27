cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.h" "*.cpp" "*.ppln" "*.samp" "*.glsl" )
if (DEFINED ANDROID)
	add_library( "Tests.ECS-st" STATIC ${SOURCES} )
else()
	add_executable( "Tests.ECS-st" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.ECS-st" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.ECS-st" PUBLIC "ECS-st" )
target_compile_definitions( "Tests.ECS-st" PUBLIC AE_TEST_ECS_ST )

if (TARGET "ResourcePacker")
	add_dependencies( "Tests.ECS-st" "ResourcePacker" )
endif ()

add_test( NAME "Tests.ECS-st" COMMAND "Tests.ECS-st" )
