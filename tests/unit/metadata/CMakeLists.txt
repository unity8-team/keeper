#
# metadata-json-test
#

set(
  METADATA_JSON_TEST
  metadata-json-test
)

add_executable(
  ${METADATA_JSON_TEST}
  metadata-json-test.cpp
)

set_target_properties(
  ${METADATA_JSON_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${METADATA_JSON_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${METADATA_JSON_TEST}
  COMMAND ${METADATA_JSON_TEST}
)

#
#
#

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${METADATA_JSON_TEST}
  PARENT_SCOPE
)
