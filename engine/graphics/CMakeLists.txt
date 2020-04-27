cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Graphics" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

set_property( TARGET "Graphics" PROPERTY FOLDER "Engine" )
target_link_libraries( "Graphics" PUBLIC "Platform" )
target_link_libraries( "Graphics" PUBLIC "Serializing" )

target_include_directories( "Graphics" PUBLIC "../../engine_tools/res_pack" )

if (${AE_ENABLE_VULKAN})
	#target_link_libraries( "Graphics" PUBLIC "Vulkan-120-lib" )
	#target_link_libraries( "Graphics" PUBLIC "Vulkan-110-lib" )
	target_link_libraries( "Graphics" PUBLIC "Vulkan-100-lib" )
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
