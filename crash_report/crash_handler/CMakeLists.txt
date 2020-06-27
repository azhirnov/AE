# You should save .pdb file or generate .sym file:
# <code>
#	add_custom_command( TARGET "<target>" POST_BUILD
#		COMMAND "${AE_BREAKPAD_DUMP_SYS}" "<exe_location>" > "<symbol_file_name>.sym"
#		COMMENT "dump syms..."
#		VERBATIM )
# </code>

if (TARGET "Breakpad-lib")
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "CrashHandler" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "CrashHandler" PROPERTY FOLDER "CrashReporting" )
	target_link_libraries( "CrashHandler" PUBLIC "STL" )
	target_link_libraries( "CrashHandler" PUBLIC "Breakpad-lib" )
	target_include_directories( "CrashHandler" PUBLIC "." )
	add_dependencies( "CrashHandler" "CrashReportSender" )
endif ()
