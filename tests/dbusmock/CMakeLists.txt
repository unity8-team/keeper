
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

set(
    TEST_NAME
    keeper-template-test
)

add_executable(
    ${TEST_NAME}
    keeper-template-test.cpp
)

target_link_libraries(
    ${TEST_NAME}
    qdbus-stubs
    ${TEST_DEPENDENCIES_LDFLAGS}
    test-utils
    Qt5::Core
    Qt5::DBus
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(
    ${TEST_NAME}
    ${TEST_NAME}
)
