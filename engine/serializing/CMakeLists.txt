cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Serializing" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Serializing" PROPERTY FOLDER "Engine" )
target_include_directories( "Serializing" PUBLIC ".." )
target_link_libraries( "Serializing" PUBLIC "Threading" )
