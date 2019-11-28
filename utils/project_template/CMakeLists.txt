cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

#----------------------------------------------------------

set( CMAKE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake" )
set( DOCS_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../docs" )

if (${AE_ENABLE_SIMPLE_COMPILER_OPTIONS})
	include( "${CMAKE_FOLDER}/compilers_minimal.cmake" )
else()
	include( "${CMAKE_FOLDER}/compilers.cmake" )
endif()

include( "${CMAKE_FOLDER}/compiler_tests.cmake" )
include( "${CMAKE_FOLDER}/ci_build.cmake" )

#----------------------------------------------------------

file( GLOB_RECURSE SOURCES "*.*" )
file( GLOB_RECURSE CMAKE_SOURCES "${CMAKE_FOLDER}/*.*" )
file( GLOB_RECURSE DOC_FILES "${DOCS_FOLDER}/*.*" )

add_library( "ProjectTemplate" STATIC EXCLUDE_FROM_ALL ${SOURCES} ${CMAKE_SOURCES} ${DOC_FILES} )
set_property( TARGET "ProjectTemplate" PROPERTY FOLDER "Utils" )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
source_group( TREE "${CMAKE_FOLDER}/.." FILES ${CMAKE_SOURCES} )
source_group( TREE "${DOCS_FOLDER}/.." FILES ${DOC_FILES} )

target_compile_definitions( "ProjectTemplate" PUBLIC ${AE_COMPILER_DEFINITIONS} )
target_link_libraries( "ProjectTemplate" PUBLIC "${AE_LINK_LIBRARIES}" )
	
# Debug
if (PROJECTS_SHARED_CXX_FLAGS_DEBUG)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
endif()
if (PROJECTS_SHARED_DEFINES_DEBUG)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_DEBUG)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
endif()

# Release
if (PROJECTS_SHARED_CXX_FLAGS_RELEASE)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
endif()
if (PROJECTS_SHARED_DEFINES_RELEASE)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_RELEASE)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
endif()

# Profile
if (PROJECTS_SHARED_DEFINES_PROFILE)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_PROFILE)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
endif()
if (PROJECTS_SHARED_CXX_FLAGS_PROFILE)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
endif()

set_target_properties( "ProjectTemplate" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( "ProjectTemplate" PUBLIC cxx_std_17 )

if (AE_CI_BUILD)
	target_compile_definitions( "ProjectTemplate" PUBLIC "AE_CI_BUILD" )
endif()

if (${AE_ENABLE_VTUNE_API})
	include( "${CMAKE_FOLDER}/vtune.cmake" )
	target_link_libraries( "ProjectTemplate" PUBLIC "VTune-lib" )
endif ()
	
if (NOT ${STD_VARIANT_SUPPORTED})
	message( FATAL_ERROR "std::variant support required" )
endif()
if (NOT ${STD_STRINGVIEW_SUPPORTED})
	message( FATAL_ERROR "std::string_view support required" )
endif()
if (NOT ${STD_OPTIONAL_SUPPORTED})
	message( FATAL_ERROR "std::optional support required" )
endif()
