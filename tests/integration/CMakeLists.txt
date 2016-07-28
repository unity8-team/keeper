set(
  INTEGRATION_TEST_LIBRARIES
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

add_subdirectory(helpers)
