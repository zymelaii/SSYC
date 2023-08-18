project(libslime)

file(GLOB_RECURSE SOURCE_FILES *.c)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
endif()

include(GNUInstallDirs)
set(PROJECT_EXPORT_TARGETS ${PROJECT_NAME})
set(PROJECT_EXPORT_NAME ${CMAKE_PROJECT_NAME})

foreach(target ${PROJECT_EXPORT_TARGETS})
    add_library(${PROJECT_EXPORT_NAME}::${target} ALIAS ${target})
endforeach()
