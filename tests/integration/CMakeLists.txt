set(
  INTEGRATION_TEST_LIBRARIES
  test-utils
  backup-helper
  storage-framework
  keeperservice
  keepertar
  qdbus-stubs
  ${SERVICE_DEPS_LDFLAGS}
  ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
  ${HELPERS_TEST_DEPS_LDFLAGS}
  ${TEST_DEPENDENCIES_LDFLAGS}
  ${GTEST_LIBRARIES}
  ${GMOCK_LIBRARIES}
)

add_subdirectory(helpers)

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  PARENT_SCOPE
)
