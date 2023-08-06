cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
	ssyc
	VERSION 0.3.1
	DESCRIPTION "Slime SysY Compiler"
	HOMEPAGE_URL "https://github.com/zymelaii/SSYC"
	LANGUAGES C CXX
)

if(MSVC)
	message(FATAL_ERROR "SSYC supports GCC/LLVM-Clang only")
endif()

option(BUILD_TEST "build test unit" OFF)
option(ENABLE_PROFILE "generate profile information" OFF)
option(USE_LIBSLIME "use libslime as standard library instead of libsysy" ON)

if(USE_LIBSLIME)
	set(STDLIB "libslime")
else()
	set(STDLIB "libsysy")
endif()

set(PROFILE_OPTIONS -fprofile-instr-generate -fcoverage-mapping)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
	add_compile_options(-g)
else()
	add_compile_options(-O2)
endif()

if(ENABLE_PROFILE)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(WARNING "profile options are now only available for clang")
		set(ENABLE_PROFILE OFF)
	endif()
endif()

add_subdirectory(src)

if(BUILD_TEST)
	add_subdirectory(test)
endif()
