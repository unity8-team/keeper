

add_definitions (-DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_definitions (-DTEST_SIMPLE_HELPER="${CMAKE_BINARY_DIR}/src/simple-helper/simple-helper")
add_definitions (-DKEEPER_SERVICE_BIN="${CMAKE_BINARY_DIR}/src/service/keeper-service")
add_definitions (-DKEEPER_CLIENT_BIN="${CMAKE_BINARY_DIR}/src/cli/keeper")

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

    tar/tar-creator-test.cpp

    storage/test-factory.cpp

    helper/helpers-test.cc
    helper/mir-mock.cpp

    metadata-providers/user-dirs-test.cpp
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
    keeperservice
    keepertar
    ${SERVICE_DEPS_LDFLAGS}
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
