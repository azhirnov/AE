cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "TestLauncher" SHARED ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

target_link_libraries( "TestLauncher" PUBLIC "Tests.ECS-st" )
target_link_libraries( "TestLauncher" PUBLIC "Tests.Networking" )
target_link_libraries( "TestLauncher" PUBLIC "Tests.Platform" )
target_link_libraries( "TestLauncher" PUBLIC "Tests.Scripting" )
target_link_libraries( "TestLauncher" PUBLIC "Tests.STL" )
target_link_libraries( "TestLauncher" PUBLIC "Tests.Threading" )
