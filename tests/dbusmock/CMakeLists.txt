
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

set(
    LINK_LIBS
    qdbus-stubs
    ${TEST_DEPENDENCIES_LDFLAGS}
    test-utils
    Qt5::Core
    Qt5::DBus
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

###
###

set(
    FOLDER_HELPER
    folder-helper
)

set(
    FOLDER_HELPER_EXEC
    ${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_HELPER}
)

add_executable(
    ${FOLDER_HELPER}
    fake-folder-helper.cpp
)
target_link_libraries(
    ${FOLDER_HELPER}
    ${LINK_LIBS}
)

###
###

set(
    TEST_NAME
    keeper-template-test
)

add_executable(
    ${TEST_NAME}
    keeper-template-test.cpp
)

set_property(
    SOURCE keeper-template-test.cpp
    PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS FOLDER_HELPER_EXEC=\"${FOLDER_HELPER_EXEC}\"
)

target_link_libraries(
    ${TEST_NAME}
    ${LINK_LIBS}
)

add_test(
    ${TEST_NAME}
    ${TEST_NAME}
)
