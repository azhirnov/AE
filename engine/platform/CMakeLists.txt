cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Platform" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

set_property( TARGET "Platform" PROPERTY FOLDER "Engine" )
target_link_libraries( "Platform" PUBLIC "Threading" )
	
if (${AE_ENABLE_GLFW})
	target_link_libraries( "Platform" PUBLIC "GLFW-lib" )
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	target_link_libraries( "Platform" PUBLIC "android" )
endif ()

if (${AE_ENABLE_VULKAN})
	target_link_libraries( "Platform" PUBLIC "Vulkan-lib" )
endif ()
