
include_directories(
  "${CMAKE_SOURCE_DIR}/src"
)

include_directories(
  SYSTEM
  ${HELPERS_TEST_DEPS_INCLUDE_DIRS}
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
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  PARENT_SCOPE
)
