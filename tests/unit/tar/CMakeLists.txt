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
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  ${TAR_CREATOR_TEST}
  ${KEEPER_TAR_CREATE_TEST}
  PARENT_SCOPE
)
