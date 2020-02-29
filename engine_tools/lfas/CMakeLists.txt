cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_executable( "LockFreeAlgSandbox" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "LockFreeAlgSandbox" PROPERTY FOLDER "EngineTools" )
target_link_libraries( "LockFreeAlgSandbox" PUBLIC "STL" )
target_include_directories( "LockFreeAlgSandbox" PUBLIC "." )
target_compile_definitions( "LockFreeAlgSandbox" PUBLIC "AE_LFAS_ENABLED" )
