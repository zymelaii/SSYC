project(slimec)

add_subdirectory(slime)
add_subdirectory(slimec)
add_subdirectory(libslime)

aux_source_directory(. SOURCE_FILES)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE ssyc::libslimec)
