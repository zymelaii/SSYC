project(unittest)

find_package(GTest REQUIRED)

aux_source_directory(. SOURCE_FILES)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(
	${PROJECT_NAME}
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE GTest::gtest GTest::gtest_main
	PRIVATE ssyc::slime
)
