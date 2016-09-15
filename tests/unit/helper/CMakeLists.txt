add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

#
# speed-test
#

set(
  SPEED_TEST
  helper-speed-test
)

add_executable(
  ${SPEED_TEST}
  speed-test.cpp
  fake-helper.h
)

set_target_properties(
  ${SPEED_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${SPEED_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

#add_test(
#  NAME ${SPEED_TEST}
#  COMMAND ${SPEED_TEST}
#)

#
#
#

#
# state-change-test
#

set(
  STATE_CHANGE_TEST
  state-change-test
)

add_executable(
  ${STATE_CHANGE_TEST}
  state-change-test.cpp
  state-test-helper.h
  state-change-test-manager.h
)

set_target_properties(
  ${STATE_CHANGE_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${STATE_CHANGE_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${STATE_CHANGE_TEST}
  COMMAND ${STATE_CHANGE_TEST}
)

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${SPEED_TEST}
  ${STATE_CHANGE_TEST}
  PARENT_SCOPE
)
