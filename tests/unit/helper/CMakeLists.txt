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

add_test(
  NAME ${SPEED_TEST}
  COMMAND ${SPEED_TEST}
)

#
#
#

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  ${SPEED_TEST}
  PARENT_SCOPE
)
