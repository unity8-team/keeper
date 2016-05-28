
include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(
    unit-tests
    storage/test-factory.cpp
)

qt5_use_modules(
    unit-tests
    Core
    DBus
    Test
)

target_link_libraries(
    unit-tests
    test-utils
    ${TEST_DEPENDENCIES_LDFLAGS}
    ${GLIB_LDFLAGS}
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(
    unit-tests
    unit-tests
)
