find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

find_package(fmt REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED COMPONENTS static)

file(GLOB_RECURSE SOURCE_FILES *.cpp)
set(YACC_SOURCE_FILES lex.yy.cpp y.tab.hpp y.tab.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
	OUTPUT ${YACC_SOURCE_FILES}
	DEPENDS ssyc/SysY.l ssyc/SysY.y
	COMMAND ${FLEX_EXECUTABLE} -f -olex.yy.cpp ${CMAKE_CURRENT_SOURCE_DIR}/ssyc/SysY.l
	COMMAND ${BISON_EXECUTABLE} --yacc -dv ${CMAKE_CURRENT_SOURCE_DIR}/ssyc/SysY.y -o y.tab.cpp
	COMMENT "Generate yacc lexer/parser"
)

add_executable(
	${CMAKE_PROJECT_NAME}
	${SOURCE_FILES}
	${YACC_SOURCE_FILES}
)

target_compile_options(
	${CMAKE_PROJECT_NAME}
	PRIVATE -Wno-register -Wno-writable-strings
)

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	PRIVATE fmt::fmt-header-only glog::glog gflags_static
)

target_include_directories(
	${CMAKE_PROJECT_NAME}
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

include(GNUInstallDirs)

install(
	TARGETS ${CMAKE_PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
