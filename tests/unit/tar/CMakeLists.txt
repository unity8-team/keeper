#
# create the wrapper scripts
#

set(
  KTC_INVOKE
  ${CMAKE_CURRENT_BINARY_DIR}/ktc-invoke.sh
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ktc-invoke.sh.in
  ${KTC_INVOKE}
)
set(
  KTC_INVOKE_NOBUS
  ${CMAKE_CURRENT_BINARY_DIR}/ktc-invoke-nobus.sh
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ktc-invoke-nobus.sh.in
  ${KTC_INVOKE_NOBUS}
)
set(
  KTC_INVOKE_NOFILES
  ${CMAKE_CURRENT_BINARY_DIR}/ktc-invoke-nofiles.sh
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ktc-invoke-nofiles.sh.in
  ${KTC_INVOKE_NOFILES}
)

add_definitions(
  -DKTC_INVOKE="${KTC_INVOKE}"
  -DKTC_INVOKE_NOBUS="${KTC_INVOKE_NOBUS}"
  -DKTC_INVOKE_NOFILES="${KTC_INVOKE_NOFILES}"
)


#
# tar-creator-test
#

set(
  TAR_CREATOR_TEST
  tar-creator-test
)

add_executable(
  ${TAR_CREATOR_TEST}
  tar-creator-test.cpp
)

target_link_libraries(
  ${TAR_CREATOR_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${TAR_CREATOR_TEST}
  ${TAR_CREATOR_TEST}
)


#
# untar-test
#

set(
  UNTAR_TEST
  untar-test
)

add_executable(
  ${UNTAR_TEST}
  untar-test.cpp
)

target_link_libraries(
  ${UNTAR_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${UNTAR_TEST}
  ${UNTAR_TEST}
)


#
# tar-creator-libarchive-failure-test
#

set(
  TAR_CREATOR_LIBARCHIVE_FAILURE_TEST
  tar-creator-libarchive-failure-test
)

add_executable(
  ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
  tar-creator-libarchive-failure-test.cpp
)

target_link_libraries(
  ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
  ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
)

foreach(funcname
        archive_errno
        archive_error_string
        archive_write_data
        archive_write_header)
  string(CONCAT LINK_FLAG "-Wl,-wrap," "${funcname}" " ")
  set_property(
    TARGET ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
    APPEND_STRING PROPERTY LINK_FLAGS ${LINK_FLAG}
  )
endforeach(funcname)

#
# keeper-tar-create-test
#

set(
  KEEPER_TAR_CREATE_TEST
  keeper-tar-create-test
)

add_executable(
  ${KEEPER_TAR_CREATE_TEST}
  keeper-tar-create-test.cpp
)

target_link_libraries(
  ${KEEPER_TAR_CREATE_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  ${KEEPER_TAR_CREATE_TEST}
  ${KEEPER_TAR_CREATE_TEST}
)

#
#
#
set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${UNTAR_TEST}
  ${TAR_CREATOR_TEST}
  ${TAR_CREATOR_LIBARCHIVE_FAILURE_TEST}
  ${KEEPER_TAR_CREATE_TEST}
  PARENT_SCOPE
)
