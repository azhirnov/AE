if (TARGET "CrashReportServer" AND TARGET "CrashReportSender")
	file( GLOB SOURCES "*.*" )
	add_executable( "Tests.CrashReportServer" ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "Tests.CrashReportServer" PROPERTY FOLDER "EngineTests" )
	target_link_libraries( "Tests.CrashReportServer" PUBLIC "STL" )
	target_link_libraries( "Tests.CrashReportServer" PUBLIC "MinidumpParser" )
	add_dependencies( "Tests.CrashReportServer" "CrashReportServer" "CrashReportSender" )
	target_compile_definitions( "Tests.CrashReportServer" PUBLIC
		"AE_CRASH_REPORT_SERVER_EXE=\"$<TARGET_FILE:CrashReportServer>\"" 
		"AE_CRASH_REPORT_SENDER_EXE=\"$<TARGET_FILE:CrashReportSender>\"" )
		
	if (NOT ${AE_CI_BUILD})
		add_test( NAME "Tests.CrashReportServer" COMMAND "Tests.CrashReportServer" )
	endif ()
endif ()
