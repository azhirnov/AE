cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "ECS-st" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "ECS-st" PROPERTY FOLDER "Engine" )
target_link_libraries( "ECS-st" PUBLIC "Platform" )
target_link_libraries( "ECS-st" PUBLIC "Graphics" )
target_link_libraries( "ECS-st" PUBLIC "Serializing" )
