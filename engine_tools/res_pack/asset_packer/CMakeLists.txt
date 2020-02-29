cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "AssetPacker" SHARED ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
#set_property( TARGET "AssetPacker" PROPERTY FOLDER "EngineTools" )
#target_link_libraries( "AssetPacker" PUBLIC "Graphics" )
target_link_libraries( "AssetPacker" PUBLIC "Serializing" )
target_link_libraries( "AssetPacker" PUBLIC "Scripting" )
target_include_directories( "AssetPacker" PUBLIC "." )

