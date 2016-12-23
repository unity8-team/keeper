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
  storage-framework
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

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${SPEED_TEST}
  PARENT_SCOPE
)
