cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
	ssyc
	VERSION 0.2.0
	DESCRIPTION "Slime SysY Compiler"
	HOMEPAGE_URL "https://github.com/zymelaii/SSYC"
	LANGUAGES C CXX
)

if(MSVC)
	message(FATAL_ERROR "SSYC supports GCC/LLVM-Clang only")
endif()

option(BUILD_TEST "build test unit" OFF)

SET(
	PROFILE_FLAGS
	-fprofile-instr-generate
	-fcoverage-mapping
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	add_compile_options(-g)
	add_link_options(${PROFILE_FLAGS})
endif()

add_subdirectory(src)

if(BUILD_TEST)
	add_subdirectory(test)
endif()
