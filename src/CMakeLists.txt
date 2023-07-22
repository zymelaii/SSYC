project(slimec)

add_subdirectory(slime)
add_subdirectory(libslime)

set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(ENABLE_PROFILE)
    target_compile_options(${PROJECT_NAME} PRIVATE ${PROFILE_OPTIONS})
    target_link_options(${PROJECT_NAME} PRIVATE ${PROFILE_OPTIONS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ssyc::slime)
