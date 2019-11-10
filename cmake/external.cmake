
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
	set( AE_EXTERNAL_TARGET "win64-msvc-v141" )
	
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
		COMMAND git lfs pull
		WORKING_DIRECTORY "${EXTERNAL_PATH}"
		RESULT_VARIABLE CHECKOUT_RESULT
	)
	#message( STATUS "${CHECKOUT_RESULT}" )
else ()
	execute_process(
		COMMAND git clone "${EXTERNAL_REPOSITORY}" "external/${AE_EXTERNAL_TARGET}" --branch "${AE_EXTERNAL_TARGET}"
		COMMAND git lfs pull
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE CLONE_RESULT
	)
	#message( STATUS "${CLONE_RESULT}" )
endif ()

add_subdirectory( "${EXTERNAL_PATH}" "external" )
