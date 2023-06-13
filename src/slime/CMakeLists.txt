project(slime)

file(GLOB_RECURSE SOURCE_FILES *.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

include(GNUInstallDirs)
set(PROJECT_EXPORT_TARGETS ${PROJECT_NAME})
set(PROJECT_EXPORT_NAME ${CMAKE_PROJECT_NAME})

foreach(target ${PROJECT_EXPORT_TARGETS})
    add_library(${PROJECT_EXPORT_NAME}::${target} ALIAS ${target})
endforeach()