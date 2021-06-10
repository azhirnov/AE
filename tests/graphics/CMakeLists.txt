file( GLOB_RECURSE SOURCES "*.h" "*.cpp" "*.ppln" "*.samp" "*.glsl" )
if (DEFINED ANDROID)
	add_library( "Tests.Graphics" STATIC ${SOURCES} )
else()
	add_executable( "Tests.Graphics" ${SOURCES} )
endif()
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Graphics" PROPERTY FOLDER "EngineTests" )
target_link_libraries( "Tests.Graphics" PUBLIC "Graphics" )
target_compile_definitions( "Tests.Graphics" PUBLIC AE_TEST_GRAPHICS )

target_compile_definitions( "Tests.Graphics" PUBLIC
	AE_PIPELINE_COMPILER_LIBRARY="${AE_PIPELINE_COMPILER_LIBRARY}"
	AE_SAMPLER_PACKER_LIBRARY="${AE_SAMPLER_PACKER_LIBRARY}"
	AE_CURRENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if (TARGET "ResourcePacker")
	add_dependencies( "Tests.Graphics" "ResourcePacker" )
endif ()

add_test( NAME "Tests.Graphics" COMMAND "Tests.Graphics" )
