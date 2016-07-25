set(KEEPER_TAR_CREATE_BIN ${CMAKE_BINARY_DIR}/src/tar/keeper-tar-create)
set(KEEPER_TAR_CREATE_INVOKE ${CMAKE_CURRENT_BINARY_DIR}/tar/keeper-tar-create-invoke.sh)

add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  -DTEST_SIMPLE_HELPER="${CMAKE_BINARY_DIR}/src/simple-helper/simple-helper"
  -DKEEPER_SERVICE_BIN="${CMAKE_BINARY_DIR}/src/service/keeper-service"
  -DKEEPER_CLIENT_BIN="${CMAKE_BINARY_DIR}/src/cli/keeper"
  -DTEST_SIMPLE_HELPER_SH="${CMAKE_SOURCE_DIR}/src/simple-helper/helper-test.sh"
  -DKEEPER_TAR_CREATE_BIN="${KEEPER_TAR_CREATE_BIN}"
  -DKEEPER_TAR_CREATE_INVOKE="${KEEPER_TAR_CREATE_INVOKE}"
)

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
)

include_directories(
  SYSTEM
  ${HELPERS_TEST_DEPS_INCLUDE_DIRS}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/tar/keeper-tar-create-invoke.sh.in
  ${KEEPER_TAR_CREATE_INVOKE}
)

set(
  UNIT_TEST_LIBRARIES
  backup-helper
  test-utils
  keeperservice
  keepertar
  qdbus-stubs
  ${SERVICE_DEPS_LDFLAGS}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${TEST_DEPENDENCIES_LDFLAGS}
  ${GTEST_LIBRARIES}
  ${GMOCK_LIBRARIES}
)

add_subdirectory(helper)
add_subdirectory(metadata-providers)
add_subdirectory(tar)

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  PARENT_SCOPE
)
