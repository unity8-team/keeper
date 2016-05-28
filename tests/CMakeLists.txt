# We add -g so we get debug info for the gtest stack frames with gdb.
# The warnings are suppressed so we get a noise-free build for gtest and gmock.
set(old_cxx_flags ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -w")
find_package(GMock REQUIRED)
find_package(Qt5Test REQUIRED)
set(CMAKE_CXX_FLAGS ${old_cxx_flags})

pkg_check_modules(TEST_DEPENDENCIES
  libqtdbustest-1 REQUIRED
  libqtdbusmock-1 REQUIRED
  libgmenuharness REQUIRED
)

include_directories(
  ${Qt5Test_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GMOCK_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIRS}
  ${TEST_DEPENDENCIES_INCLUDE_DIRS}
)

add_subdirectory(copyright)
add_subdirectory(unit)
add_subdirectory(utils)
add_subdirectory(whitespace)

add_test(cppcheck cppcheck --enable=all -USCHEMA_DIR --error-exitcode=2 --inline-suppr -I${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
