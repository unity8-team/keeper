set(
  TEST_NAME
  tar-creator-test
)

add_executable(
  ${TEST_NAME}
  tar-creator-test.cpp
)

target_link_libraries(
  ${TEST_NAME}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${TEST_NAME}
  ${TEST_NAME}
)

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  ${TEST_NAME}
  PARENT_SCOPE
)
