project(slimec)

add_subdirectory(slime)
add_subdirectory(libslime)

set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ssyc::slime)
