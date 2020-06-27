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

if (${AE_ENABLE_OPENVR})
	target_link_libraries( "Platform" PUBLIC "OpenVR-lib" )
endif ()

if (${AE_ENABLE_OPENXR})
	target_link_libraries( "Platform" PUBLIC "OpenXR-lib" )
endif ()
