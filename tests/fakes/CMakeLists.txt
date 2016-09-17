
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/tests/
)

set(
    LINK_LIBS
    qdbus-stubs
    ${TEST_DEPENDENCIES_LDFLAGS}
    test-utils
    Qt5::Core
    Qt5::DBus
    Qt5::Network
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

###
###

add_executable(
    ${BACKUP_HELPER}
    fake-backup-helper.cpp
)
target_link_libraries(
    ${BACKUP_HELPER}
    ${LINK_LIBS}
)

add_executable(
    ${BACKUP_HELPER_FAILURE}
    fake-backup-helper.cpp
)
target_compile_definitions(
    ${BACKUP_HELPER_FAILURE}
    PRIVATE
    -DCOMPILE_WITH_FAILURE
)

target_link_libraries(
    ${BACKUP_HELPER_FAILURE}
    ${LINK_LIBS}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/helper-test.sh.in
  ${KEEPER_HELPER_TEST_LOCATION}
)

add_subdirectory(upstart)
