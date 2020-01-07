cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "SamplerPacker" SHARED ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
target_link_libraries( "SamplerPacker" PRIVATE "Serializing" )
target_link_libraries( "SamplerPacker" PRIVATE "Scripting" )
target_compile_definitions( "SamplerPacker" PRIVATE "AE_BUILD_SAMPLER_PACKER" )
target_include_directories( "SamplerPacker" PRIVATE "." )
