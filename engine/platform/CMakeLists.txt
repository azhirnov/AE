cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	file( GLOB_RECURSE ANDROID_SOURCES "${MAIN_SOURCE_DIR}/android/engine/src/*.java" )
else ()
	set( ANDROID_SOURCES "" )
endif ()

add_library( "Platform" STATIC ${SOURCES} ${ANDROID_SOURCES} )

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
source_group( TREE ${MAIN_SOURCE_DIR} FILES ${ANDROID_SOURCES} )

set_property( TARGET "Platform" PROPERTY FOLDER "Engine" )
target_link_libraries( "Platform" PUBLIC "STL" )

if (${AE_ENABLE_VULKAN})
	target_link_libraries( "Platform" PUBLIC "Vulkan-lib" )
endif ()
