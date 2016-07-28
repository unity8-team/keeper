###
###  Build Google Test and Google Mock
###

# We add -g so we get debug info for the gtest stack frames with gdb.
# We add -w to supress warnings when compiling this external source code.
set(old_cxx_flags ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -w")
find_package(GMock REQUIRED)
find_package(Qt5Test REQUIRED)
set(CMAKE_CXX_FLAGS ${old_cxx_flags})

###
###  Service DBusMock
###

set(
    BACKUP_HELPER
    fake-backup-helper
)

set(KEEPER_TAR_CREATE_BIN ${CMAKE_BINARY_DIR}/src/tar/keeper-tar-create)
set(KEEPER_TAR_CREATE_INVOKE ${CMAKE_CURRENT_BINARY_DIR}/tar/keeper-tar-create-invoke.sh)

add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  -DTEST_SIMPLE_HELPER="${CMAKE_BINARY_DIR}/tests/fakes/${BACKUP_HELPER}"
  -DKEEPER_SERVICE_BIN="${CMAKE_BINARY_DIR}/src/service/keeper-service"
  -DKEEPER_CLIENT_BIN="${CMAKE_BINARY_DIR}/src/cli/keeper"
  -DTEST_SIMPLE_HELPER_SH="${CMAKE_SOURCE_DIR}/tests/fakes/helper-test.sh"
  -DKEEPER_TAR_CREATE_BIN="${KEEPER_TAR_CREATE_BIN}"
  -DKEEPER_TAR_CREATE_INVOKE="${KEEPER_TAR_CREATE_INVOKE}"
)

set(SERVICE_TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/com_canonical_keeper.py")
add_definitions(-DSERVICE_TEMPLATE_FILE="${SERVICE_TEMPLATE_FILE}")

# get the python3 package directory
execute_process (
  COMMAND python3 -c "from distutils import sysconfig; print(sysconfig.get_python_lib())"
  COMMAND sed -r -e "s|/usr/(local/)?||g"
  OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)

#install(
#  FILES ${SERVICE_TEMPLATE_FILE}
#  DESTINATION "${PYTHON_PACKAGE_DIR}/dbusmock/templates/"
#)

###
###  The Tests
###

pkg_check_modules(TEST_DEPENDENCIES
  libqtdbustest-1 REQUIRED
  libqtdbusmock-1 REQUIRED
)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src
  ${Qt5Test_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GMOCK_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIRS}
  ${TEST_DEPENDENCIES_INCLUDE_DIRS}
)

add_subdirectory(surface)
add_subdirectory(dbusmock)
add_subdirectory(unit)
add_subdirectory(utils)
add_subdirectory(fakes)
add_subdirectory(integration)

set(
  UNIT_TEST_TARGETS
  ${UNIT_TEST_TARGETS}
  PARENT_SCOPE
)
