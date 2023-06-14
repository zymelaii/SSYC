project(slimec)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_options(-g)
endif()

add_subdirectory(slime)

set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ssyc::slime)
