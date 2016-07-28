
#
# helpers-test
#

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

set(
  HELPERS_TEST
  helpers-test
)

add_executable(
  ${HELPERS_TEST}
  helpers-test.cc
  mir-mock.cpp
)

set_target_properties(
  ${HELPERS_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${HELPERS_TEST}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${INTEGRATION_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${HELPERS_TEST}
  COMMAND ${HELPERS_TEST}
)

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  ${HELPERS_TEST}
  PARENT_SCOPE
)
