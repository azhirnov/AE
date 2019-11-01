
# detect target platform
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	if (NOT MSVC)
		message( FATAL_ERROR "Microsowft Visual Studio compiler is required" )
	endif ()
	if (NOT ${CMAKE_GENERATOR_TOOLSET} STREQUAL "v141")
		message( FATAL_ERROR "Toolset ${CMAKE_GENERATOR_TOOLSET} is not supported, required 'v141' toolset" )
	endif ()
	if (NOT ${PLATFORM_BITS} EQUAL 64)
		message( FATAL_ERROR "64-bit Windows platform is required" )
	endif ()
	set( AE_EXTERNAL_TARGET "win64-msvc-${CMAKE_GENERATOR_TOOLSET}" )
else ()
    message( FATAL_ERROR "unsupported target platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}" )
endif ()

# download
set( EXTERNAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external" )

if (CMAKE_VERSION VERSION_LESS 3.11.0)

	include( ExternalProject )
	
	ExternalProject_Add( ExternalDependencies
		# download
		GIT_REPOSITORY		"https://github.com/azhirnov/AE-External.git"
		GIT_TAG				${AE_EXTERNAL_TARGET}
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
		SOURCE_DIR			"${EXTERNAL_PATH}"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)

else ()

	include( FetchContent )

	#set( FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "don't download externals" )
	set( FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "don't update externals" )
	mark_as_advanced( FETCHCONTENT_BASE_DIR FETCHCONTENT_FULLY_DISCONNECTED )
	mark_as_advanced( FETCHCONTENT_QUIET FETCHCONTENT_UPDATES_DISCONNECTED )

	FetchContent_Declare( ExternalDependencies
		GIT_REPOSITORY		"https://github.com/azhirnov/AE-External.git"
		GIT_TAG				"${AE_EXTERNAL_TARGET}"
		# configure
		SOURCE_DIR			"${EXTERNAL_PATH}"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
	FetchContent_GetProperties( ExternalDependencies )
	if (NOT ExternalDependencies_POPULATED)
		message( STATUS "downloading external dependencies" )
		FetchContent_Populate( ExternalDependencies )
	endif ()

endif ()

add_subdirectory( ${EXTERNAL_PATH} "external" )
