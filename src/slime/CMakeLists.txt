project(slime)

file(GLOB_RECURSE SOURCE_FILES *.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

if(STDLIB STREQUAL "libslime")
    target_compile_definitions(
        ${PROJECT_NAME} PRIVATE
        USE_LIBSLIME
        SLIMEC_STDLIB="${STDLIB}"
        SLIMEC_VERSION="${CMAKE_PROJECT_VERSION}"
        SLIMEC_HOMEPAGEURL="${CMAKE_PROJECT_HOMEPAGE_URL}"
    )
endif()

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
target_include_directories(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

if(ENABLE_PROFILE)
    target_compile_options(${PROJECT_NAME} PRIVATE ${PROFILE_FLAGS})
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format)

include(GNUInstallDirs)
set(PROJECT_EXPORT_TARGETS ${PROJECT_NAME})
set(PROJECT_EXPORT_NAME ${CMAKE_PROJECT_NAME})

foreach(target ${PROJECT_EXPORT_TARGETS})
    add_library(${PROJECT_EXPORT_NAME}::${target} ALIAS ${target})
endforeach()
