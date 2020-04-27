cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "UI" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

set_property( TARGET "UI" PROPERTY FOLDER "Engine" )
target_link_libraries( "UI" PUBLIC "Graphics" )
