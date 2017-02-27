
add_definitions(
  -DPROJECT_NAME="${CMAKE_PROJECT_NAME}"
  -DHELPER_REGISTRY_FILENAME="${HELPER_REGISTRY_FILENAME}"
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
  "${CMAKE_BINARY_DIR}/tests/qdbus-stubs"
)

set(
  HELPERS_TEST
  helpers-test
)

add_executable(
  ${HELPERS_TEST}
  ${interface_files}
  helpers-test.cc
  test-helpers-base.cpp
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
  qdbus-stubs-tests
  Qt5::DBus
  Qt5::Test
  Qt5::Network
  Qt5::Core
)

add_test(
  NAME ${HELPERS_TEST}
  COMMAND ${HELPERS_TEST}
)

set(
  FOLDER_BACKUP_EXEC
  ${KEEPER_HELPER_TEST_LOCATION}
)
set(
  FOLDER_RESTORE_EXEC
  ${RESTORE_HELPER_TEST_LOCATION}
)
configure_file(
  ${CMAKE_SOURCE_DIR}/data/${HELPER_REGISTRY_FILENAME}.in
  ${HELPERS_TEST}-registry.json
  @ONLY
)
set_property(
  TARGET ${HELPERS_TEST}
  APPEND PROPERTY COMPILE_DEFINITIONS HELPER_REGISTRY="${CMAKE_CURRENT_BINARY_DIR}/${HELPERS_TEST}-registry.json"
)

#
#   restore-test
#
set(
  RESTORE_TEST
  restore-test
)

add_executable(
  ${RESTORE_TEST}
  ${interface_files}
  restore-test.cpp
  test-helpers-base.cpp
)

set_target_properties(
  ${RESTORE_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${RESTORE_TEST}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${INTEGRATION_TEST_LIBRARIES}
  qdbus-stubs-tests
  Qt5::DBus
  Qt5::Test
  Qt5::Network
  Qt5::Core
)

add_test(
  NAME ${RESTORE_TEST}
  COMMAND ${RESTORE_TEST}
)

set(
  FOLDER_BACKUP_EXEC
  ${KEEPER_HELPER_TEST_LOCATION}
)

set_property(
  TARGET ${RESTORE_TEST}
  APPEND PROPERTY COMPILE_DEFINITIONS HELPER_REGISTRY="${CMAKE_CURRENT_BINARY_DIR}/${HELPERS_TEST}-registry.json"
)

#
#  helpers-test-failure
#

set(
  HELPERS_TEST_FAILURE
  helpers-test-failure
)

add_executable(
  ${HELPERS_TEST_FAILURE}
  helpers-test-failure.cpp
  test-helpers-base.cpp
)

set_target_properties(
  ${HELPERS_TEST_FAILURE}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

set(
  FOLDER_BACKUP_EXEC
  ${BACKUP_HELPER_FAILURE_LOCATION}
)
configure_file(
  ${CMAKE_SOURCE_DIR}/data/${HELPER_REGISTRY_FILENAME}.in
  ${HELPERS_TEST_FAILURE}-registry.json
  @ONLY
)
set_property(
  TARGET ${HELPERS_TEST_FAILURE}
  APPEND PROPERTY COMPILE_DEFINITIONS HELPER_REGISTRY="${CMAKE_CURRENT_BINARY_DIR}/${HELPERS_TEST_FAILURE}-registry.json"
)

target_link_libraries(
  ${HELPERS_TEST_FAILURE}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${INTEGRATION_TEST_LIBRARIES}
  qdbus-stubs-tests
  Qt5::DBus
  Qt5::Test
  Qt5::Network
  Qt5::Core
)

add_test(
  NAME ${HELPERS_TEST_FAILURE}
  COMMAND ${HELPERS_TEST_FAILURE}
)

#
#  state-change-test
#

set(
  HELPERS_STATE_CHANGE
  helpers-state-change
)

add_executable(
  ${HELPERS_STATE_CHANGE}
  state-change-test.cpp
  test-helpers-base.cpp
  state-change-test-manager.h
  state-test-helper.h
)

set_target_properties(
  ${HELPERS_STATE_CHANGE}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)


target_link_libraries(
  ${HELPERS_STATE_CHANGE}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${INTEGRATION_TEST_LIBRARIES}
  qdbus-stubs-tests
  Qt5::DBus
  Qt5::Test
  Qt5::Network
  Qt5::Core
)

add_test(
  NAME ${HELPERS_STATE_CHANGE}
  COMMAND ${HELPERS_STATE_CHANGE}
)


#
#
#

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${HELPERS_TEST}
  ${HELPERS_TEST_FAILURE}
  ${HELPERS_STATE_CHANGE}
  PARENT_SCOPE
)
