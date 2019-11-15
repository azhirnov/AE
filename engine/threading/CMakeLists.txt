cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Threading" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Threading" PROPERTY FOLDER "Engine" )
target_link_libraries( "Threading" PUBLIC "STL" )
set_target_properties( "Threading" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
