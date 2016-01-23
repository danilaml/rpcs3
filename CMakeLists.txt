cmake_minimum_required(VERSION 2.8.12)

set(ASMJIT_STATIC TRUE)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message( FATAL_ERROR "RPCS3 can only be compiled on 64-bit platforms." )
endif()

message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_PREFIX_PATH})
set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets/src/zlib") # We use the zlib, that's in wxWidgets

add_subdirectory( asmjit )
add_subdirectory( libpng )
# TODO: do real installation, including copying directory structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/bin")
add_subdirectory( rpcs3 )
