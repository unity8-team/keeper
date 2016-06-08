# We add -g so we get debug info for the gtest stack frames with gdb.
# The warnings are suppressed so we get a noise-free build for gtest and gmock.

set(old_cxx_flags ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -w")
find_package(GMock)
set(CMAKE_CXX_FLAGS ${old_cxx_flags})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(unit_test_dirs
    helper
)

set(slow_test_dirs
)

set(UNIT_TEST_TARGETS "")

foreach(dir ${unit_test_dirs})
    add_subdirectory(${dir})
    list(APPEND UNIT_TEST_TARGETS "${dir}_test")
endforeach()

if (${slowtests})
    foreach(dir ${slow_test_dirs})
        add_subdirectory(${dir})
        list(APPEND UNIT_TEST_TARGETS "${dir}_test")
    endforeach()
    add_subdirectory(headers)
endif()

set(UNIT_TEST_TARGETS ${UNIT_TEST_TARGETS} PARENT_SCOPE)

if (${slowtests})
    add_subdirectory(copyright)
endif()
add_subdirectory(whitespace)
