

add_definitions (-DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

pkg_check_modules(HELPERS_TEST_DEPS REQUIRED
    ubuntu-app-launch-2
    properties-cpp
    dbustest-1>=14.04.0
    glib-2.0>=2.26
    gio-2.0>=2.26
    gio-unix-2.0
)

include_directories(
    "${CMAKE_SOURCE_DIR}/src"
)

include_directories(
    SYSTEM
    ${HELPERS_TEST_DEPS_INCLUDE_DIRS}
)

add_executable(
    unit-tests

    storage/test-factory.cpp

    helper/helpers-test.cc
    helper/mir-mock.cpp
)

qt5_use_modules(
    unit-tests
    Core
    DBus
    Test
)

target_link_libraries(
    unit-tests
    backup-helper
    test-utils
    ${HELPERS_TEST_DEPS_LDFLAGS}
    ${TEST_DEPENDENCIES_LDFLAGS}
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(
    unit-tests
    unit-tests
)

add_subdirectory(helper)
