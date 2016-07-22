
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
    Qt5::Network
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

###
###

set(
    BACKUP_HELPER
    fake-backup-helper
)

add_executable(
    ${BACKUP_HELPER}
    fake-backup-helper.cpp
)
target_link_libraries(
    ${BACKUP_HELPER}
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
    PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS FAKE_BACKUP_HELPER_EXEC=\"${CMAKE_CURRENT_BINARY_DIR}/${BACKUP_HELPER}\"
)

target_link_libraries(
    ${TEST_NAME}
    ${LINK_LIBS}
)

add_test(
    ${TEST_NAME}
    ${TEST_NAME}
)
