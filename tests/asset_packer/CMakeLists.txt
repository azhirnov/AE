file( GLOB_RECURSE SOURCES "*.*" )
add_executable( "Tests.AssetPacker" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.AssetPacker" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.AssetPacker" PUBLIC "STL" )
target_include_directories( "Tests.AssetPacker" PRIVATE "../../engine_tools/res_pack" )
	
target_compile_definitions( "Tests.AssetPacker" PUBLIC
	AE_PIPELINE_COMPILER_LIBRARY="${AE_PIPELINE_COMPILER_LIBRARY}"
	AE_SAMPLER_PACKER_LIBRARY="${AE_SAMPLER_PACKER_LIBRARY}"
	AE_CURRENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if (TARGET "ResourcePacker")
	add_dependencies( "Tests.AssetPacker" "ResourcePacker" )
endif ()

add_test( NAME "Tests.AssetPacker" COMMAND "Tests.AssetPacker" )
