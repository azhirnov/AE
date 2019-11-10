cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "STL" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "STL" PROPERTY FOLDER "Engine" )
target_include_directories( "STL" PUBLIC ".." )
target_link_libraries( "STL" PUBLIC "ProjectTemplate" )
target_link_libraries( "STL" PUBLIC "GLM-lib" )
set_target_properties( "STL" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_link_libraries( "STL" PUBLIC "dl;pthread" )
	target_compile_definitions( "STL" PUBLIC _LARGEFILE_SOURCE )
	target_compile_definitions( "STL" PUBLIC _LARGE_FILES )
	target_compile_definitions( "STL" PUBLIC _FILE_OFFSET_BITS=64 )
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	target_link_libraries( "STL" PUBLIC "log" )
endif ()

install( TARGETS "STL" ARCHIVE DESTINATION "lib" )
