cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
	ssyc
	VERSION 0.5.1
	DESCRIPTION "Slime SysY Compiler"
	HOMEPAGE_URL "https://github.com/zymelaii/SSYC"
	LANGUAGES C CXX
)

option(BUILD_TEST "build test unit" OFF)
option(USE_LIBSLIME "use libslime as standard library instead of libsysy" ON)

if(USE_LIBSLIME)
	set(STDLIB "libslime")
else()
	set(STDLIB "libsysy")
endif()

add_subdirectory(src)

if(BUILD_TEST)
	add_subdirectory(test)
endif()
