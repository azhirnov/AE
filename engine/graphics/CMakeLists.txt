file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Graphics" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

set_property( TARGET "Graphics" PROPERTY FOLDER "Engine" )
target_link_libraries( "Graphics" PUBLIC "Platform" )
target_link_libraries( "Graphics" PUBLIC "Serializing" )

target_include_directories( "Graphics" PUBLIC "../../engine_tools/res_pack" )

if (${AE_ENABLE_VULKAN})
	set( AE_VULKAN_VERSION "110" CACHE STRING "Vulkan API version" )
	mark_as_advanced( AE_VULKAN_VERSION )
	
	if (NOT TARGET "Vulkan-${AE_VULKAN_VERSION}-lib")
		message( FATAL_ERROR "unsupported Vulkan API version '${AE_VULKAN_VERSION}'" )
	else ()
		message( STATUS "Vulkan API version: '${AE_VULKAN_VERSION}'" )
	endif ()
	target_link_libraries( "Graphics" PUBLIC "Vulkan-${AE_VULKAN_VERSION}-lib" )
endif ()

if (${AE_ENABLE_VMA})
	target_link_libraries( "Graphics" PUBLIC "VMA-lib" )
endif ()

if (${AE_ENABLE_OPENGL})
	target_link_libraries( "Graphics" PUBLIC "OpenGL-lib" )
endif ()

if (${AE_ENABLE_FREETYPE})
	target_link_libraries( "Graphics" PUBLIC "FreeType-lib" )
endif ()
