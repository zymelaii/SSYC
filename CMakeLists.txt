cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
	SSYC
	VERSION 0.1.0
	DESCRIPTION "Slime SysY Compiler"
	HOMEPAGE_URL "https://github.com/zymelaii/SSYC"
	LANGUAGES C CXX
)

if(MSVC)
	message(FATAL_ERROR "SSYC supports GCC/LLVM-Clang only")
endif()

option(BUILD_TEST "build test unit" ON)

add_subdirectory(src)

if(BUILD_TEST)
	add_subdirectory(test)
endif()
