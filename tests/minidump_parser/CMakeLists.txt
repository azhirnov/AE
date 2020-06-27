file( GLOB SOURCES "*.*" )
add_executable( "Tests.MinidumpParser" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.MinidumpParser" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.MinidumpParser" PUBLIC "MinidumpParser" )
	
add_test( NAME "Tests.MinidumpParser" COMMAND "Tests.MinidumpParser" )
