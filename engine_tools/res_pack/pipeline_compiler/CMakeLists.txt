cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "PipelineCompiler" SHARED ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
#set_property( TARGET "PipelineCompiler" PROPERTY FOLDER "EngineTools" )
target_link_libraries( "PipelineCompiler" PRIVATE "Serializing" )
target_link_libraries( "PipelineCompiler" PRIVATE "Scripting" )
target_compile_definitions( "PipelineCompiler" PRIVATE "AE_BUILD_PIPELINE_COMPILER" )
target_include_directories( "PipelineCompiler" PRIVATE "." )

if ((NOT ${AE_ENABLE_GLSLANG}) OR (NOT ${AE_ENABLE_SPIRV_CROSS}))
	message( FATAL_ERROR "GLSLang and SPIRV-Cross are required" )
endif ()

target_link_libraries( "PipelineCompiler" PRIVATE "GLSLang-lib" )
target_link_libraries( "PipelineCompiler" PRIVATE "SPIRV-Cross-lib" )
