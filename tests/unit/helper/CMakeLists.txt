add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

set(
  TEST_NAME
  helpers-test
)

pkg_check_modules(HELPERS_TEST_DEPS REQUIRED
  ubuntu-app-launch-2
  properties-cpp
  dbustest-1>=14.04.0
  glib-2.0>=2.26
  gio-2.0>=2.26
  gio-unix-2.0
)
include_directories(
  SYSTEM
  ${UAL_INCLUDE_DIRS}
  ${PROP-CPP_INCLUDE_DIRS}
  ${HELPERS_TEST_DEPS_INCLUDE_DIRS}
)

add_executable(
  ${TEST_NAME}
  helpers-test.cc
  mir-mock.cpp
)

set_target_properties(
  ${TEST_NAME}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${TEST_NAME}
  gtest
  backup-helper
  test-utils
  ${HELPERS_TEST_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${TEST_NAME}
  COMMAND ${TEST_NAME}
)

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  ${TEST_NAME}
  PARENT_SCOPE
)
