cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_executable( "Tests.AssetPacker" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.AssetPacker" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.AssetPacker" PUBLIC "STL" )

target_include_directories( "Tests.AssetPacker" PRIVATE "../../engine_tools/res_pack" )
	
add_test( NAME "Tests.AssetPacker" COMMAND "Tests.AssetPacker" )
