
# detect target platform
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	if (NOT MSVC)
		message( FATAL_ERROR "Microsowft Visual Studio compiler is required" )
	endif ()
	if (NOT (${CMAKE_SIZEOF_VOID_P} EQUAL 8))
		message( FATAL_ERROR "64-bit Windows platform is required" )
	endif ()
	if (NOT (${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE} STREQUAL "x64"))
		message( FATAL_ERROR "64-bit compiler is required" )
	endif ()
	if (NOT (${CMAKE_VS_PLATFORM_TOOLSET} STREQUAL "v141") AND
		NOT (${CMAKE_VS_PLATFORM_TOOLSET} STREQUAL "v142"))
		message( FATAL_ERROR "Toolset ${CMAKE_VS_PLATFORM_TOOLSET} is not supported, required 'v141' or 'v142'" )
	endif ()
	set( AE_EXTERNAL_TARGET "win64-msvc-${CMAKE_VS_PLATFORM_TOOLSET}" )
	
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set( AE_EXTERNAL_TARGET "android" )

else ()
	message( FATAL_ERROR "unsupported target platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}" )
endif ()

set( EXTERNAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/${AE_EXTERNAL_TARGET}" )
set( EXTERNAL_REPOSITORY "https://github.com/azhirnov/AE-External.git" )

file( MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external" )


# download
find_package( Git )
if (NOT Git_FOUND)
	message( FATAL_ERROR "Git not found" )
endif ()
	
file( MAKE_DIRECTORY "${EXTERNAL_PATH}" )
	
if (EXISTS "${EXTERNAL_PATH}/CMakeLists.txt")
	execute_process(
		COMMAND git checkout "${AE_EXTERNAL_TARGET}"
		WORKING_DIRECTORY "${EXTERNAL_PATH}"
	)
else ()
	execute_process(
		COMMAND git clone "${EXTERNAL_REPOSITORY}" "external/${AE_EXTERNAL_TARGET}" --branch "${AE_EXTERNAL_TARGET}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
endif ()

execute_process(
	COMMAND git lfs pull
	WORKING_DIRECTORY "${EXTERNAL_PATH}"
)

add_subdirectory( "${EXTERNAL_PATH}" "external" )
