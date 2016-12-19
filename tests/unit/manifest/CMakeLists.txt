add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

#
# manifest-test
#

set(
  MANIFEST_TEST
  manifest-test
)

add_executable(
  ${MANIFEST_TEST}
  manifest-test.cpp
)

set_target_properties(
  ${MANIFEST_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${MANIFEST_TEST}
  ${UNIT_TEST_LIBRARIES}
  backup-helper
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${MANIFEST_TEST}
  COMMAND ${MANIFEST_TEST}
)

#
#
#

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${MANIFEST_TEST}
  PARENT_SCOPE
)
