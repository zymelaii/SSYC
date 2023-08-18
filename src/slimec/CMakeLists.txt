project(libslimec)

set(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-static)

file(GLOB_RECURSE SOURCE_FILES *.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
add_library(${PROJECT_NAME}-static STATIC ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME slimec)
set_target_properties(${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME slimec-static)

foreach(target ${TARGETS})
    target_include_directories(
        ${target}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(${target} PUBLIC ssyc::slime)
endforeach()

include(GNUInstallDirs)
set(PROJECT_EXPORT_TARGETS ${TARGETS})
set(PROJECT_EXPORT_NAME ${CMAKE_PROJECT_NAME})

foreach(target ${PROJECT_EXPORT_TARGETS})
    add_library(${PROJECT_EXPORT_NAME}::${target} ALIAS ${target})
endforeach()
