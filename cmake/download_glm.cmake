# find or download glm (MIT license)

set( AE_EXTERNAL_GLM_PATH "" CACHE PATH "path to glm source" )
mark_as_advanced( AE_EXTERNAL_GLM_PATH )

# reset to default
if (NOT EXISTS "${AE_EXTERNAL_GLM_PATH}/glm/glm.hpp")
	message( STATUS "glm is not found in \"${AE_EXTERNAL_GLM_PATH}\"" )
	set( AE_EXTERNAL_GLM_PATH "${AE_EXTERNALS_PATH}/glm" CACHE PATH "" FORCE )
else ()
	message( STATUS "glm found in \"${AE_EXTERNAL_GLM_PATH}\"" )
endif ()
	
# select version
if (${AE_EXTERNALS_USE_STABLE_VERSIONS})
	set( GLM_TAG "0.9.9.5" )
else ()
	set( GLM_TAG "master" )
endif ()

if (NOT EXISTS "${AE_EXTERNAL_GLM_PATH}/glm/glm.hpp")
	set( AE_GLM_REPOSITORY "https://github.com/g-truc/glm.git" )
else ()
	set( AE_GLM_REPOSITORY "" )
endif ()

ExternalProject_Add( "External.GLM"
	LIST_SEPARATOR		"${AE_LIST_SEPARATOR}"
	# download
	GIT_REPOSITORY		${AE_GLM_REPOSITORY}
	GIT_TAG				${GLM_TAG}
	GIT_PROGRESS		1
	EXCLUDE_FROM_ALL	1
	LOG_DOWNLOAD		1
	# update
	PATCH_COMMAND		""
	UPDATE_DISCONNECTED	1
	# configure
	SOURCE_DIR			"${AE_EXTERNAL_GLM_PATH}"
	CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
	CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
	CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
	CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${AE_EXTERNAL_CONFIGURATION_TYPES}"
						"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
						"-DCMAKE_DEBUG_POSTFIX="
						"-DCMAKE_RELEASE_POSTFIX="
						${AE_BUILD_TARGET_FLAGS}
	LOG_CONFIGURE 		1
	# build
	BINARY_DIR			"${CMAKE_BINARY_DIR}/build-glm"
	BUILD_COMMAND		""
	LOG_BUILD 			1
	# install
	INSTALL_DIR 		""
	INSTALL_COMMAND		""
	LOG_INSTALL 		1
	# test
	TEST_COMMAND		""
)
	
set_property( TARGET "External.GLM" PROPERTY FOLDER "External" )

add_library( "GLM-lib" INTERFACE )
target_include_directories( "GLM-lib" INTERFACE "${AE_EXTERNAL_GLM_PATH}/glm" )
target_compile_definitions( "GLM-lib" INTERFACE "AE_ENABLE_GLM" )
add_dependencies( "GLM-lib" "External.GLM" )
