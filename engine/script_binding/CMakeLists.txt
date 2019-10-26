cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

if (${AE_ENABLE_ANGELSCRIPT})
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "ScriptBinding" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_link_libraries( "ScriptBinding" "STL" )
	target_link_libraries( "ScriptBinding" "AngelScript-lib" )
endif()
