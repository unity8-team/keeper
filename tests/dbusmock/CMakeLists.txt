
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

###
###

set(
    TEST_NAME
    keeper-template-test
)

add_executable(
    ${TEST_NAME}
    keeper-template-test.cpp
)

set_property(
    SOURCE keeper-template-test.cpp
    PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS FAKE_BACKUP_HELPER_EXEC=\"${CMAKE_BINARY_DIR}/tests/fakes/${BACKUP_HELPER}\"
)

target_link_libraries(
    ${TEST_NAME}
    test-utils
    backup-helper
    storage-framework
    qdbus-stubs
    util
    ${TEST_DEPENDENCIES_LDFLAGS}
    ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
    Qt5::Core
    Qt5::DBus
    Qt5::Network
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(
    ${TEST_NAME}
    ${TEST_NAME}
)

#
#
#
set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${TEST_NAME}
  PARENT_SCOPE
)
