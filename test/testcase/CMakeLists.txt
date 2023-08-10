macro(get_directories RESULT PARENT)
    file(GLOB ITEMS RELATIVE ${PARENT} ${PARENT}/*)
    set(DIRLIST)
    foreach(ITEM ${ITEMS})
        if(IS_DIRECTORY ${PARENT}/${ITEM})
        list(APPEND DIRLIST ${ITEM})
        endif()
    endforeach()
    set(${RESULT} ${DIRLIST})
endmacro()

get_directories(CATEGORY_LIST ${CMAKE_CURRENT_SOURCE_DIR})

set(SLIME_STDLIB ssyc::libslime)
get_target_property(LIBSLIME_INCLUDEDIR ${SLIME_STDLIB} INCLUDE_DIRECTORIES)
get_target_property(LIBSLIME_LIBDIR ${SLIME_STDLIB} BINARY_DIR)
get_target_property(LIBSLIME ${SLIME_STDLIB} NAME)

set(CATEGORY_TARGET_LIST)
foreach(CATEGORY ${CATEGORY_LIST})
    file(GLOB SYSY_SOURCES ${CATEGORY}/*.sy)
    set(CATEGORY_TESTCASES)
    foreach(SOURCE ${SYSY_SOURCES})
        get_filename_component(TESTCASE ${SOURCE} NAME_WLE)
        string(CONCAT TARGET ${CATEGORY} - ${TESTCASE})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CATEGORY})
        add_custom_target(
            ${TARGET}
            DEPENDS slimec ${SLIME_STDLIB}
            SOURCES ${SOURCE}
            COMMENT "Building standard program for testcase ${CATEGORY}/${TESTCASE}"
            COMMAND gcc -w -m32 -Ui386 -O2
                -include ${LIBSLIME_INCLUDEDIR}/libslime/stdslime.h
                -x c ${SOURCE}
                -L${LIBSLIME_LIBDIR}
                -l${LIBSLIME}
                -o ${CMAKE_CURRENT_BINARY_DIR}/${CATEGORY}/${TESTCASE}
        )
        list(APPEND CATEGORY_TESTCASES ${TARGET})
    endforeach()
    string(CONCAT CATEGORY_TARGET category - ${CATEGORY})
    list(APPEND CATEGORY_TARGET_LIST ${CATEGORY_TARGET})
    add_custom_target(${CATEGORY_TARGET} DEPENDS ${CATEGORY_TESTCASES})
endforeach()

add_custom_target(testcase DEPENDS ${CATEGORY_TARGET_LIST})
