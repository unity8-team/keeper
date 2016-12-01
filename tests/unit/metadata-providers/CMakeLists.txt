set(
  TEST_NAME
  user-dirs-test
)

add_executable(
  ${TEST_NAME}
  user-dirs-test.cpp
)

target_link_libraries(
  ${TEST_NAME}
  ${UNIT_TEST_LIBRARIES}
  storage-framework
  backup-helper
  ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${TEST_NAME}
  ${TEST_NAME}
)

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${TEST_NAME}
  PARENT_SCOPE
)
