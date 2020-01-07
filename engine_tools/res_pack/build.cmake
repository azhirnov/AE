
include( ExternalProject )

ExternalProject_Add( "ResourcePacker"
		LOG_OUTPUT_ON_FAILURE 1
		LIST_SEPARATOR		"|"
		DOWNLOAD_COMMAND	""
		# configure
		SOURCE_DIR			"${CMAKE_CURRENT_SOURCE_DIR}/engine_tools/res_pack"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=Profile|Release"
							"-DAE_ENABLE_TESTS=${AE_ENABLE_TESTS}"
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_CURRENT_BINARY_DIR}/build-ResourcePacker"
		BUILD_COMMAND		"${CMAKE_COMMAND}"
							--build .
							--config Profile
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		INSTALL_COMMAND		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND 		""
	)
set_property( TARGET "ResourcePacker" PROPERTY FOLDER "EngineTools" )
