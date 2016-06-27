set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions ( -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )
add_definitions ( -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )

include(FindPkgConfig)
pkg_check_modules(UAL REQUIRED ubuntu-app-launch-2)
pkg_check_modules(PROP-CPP REQUIRED properties-cpp)

pkg_check_modules(DBUSTEST REQUIRED dbustest-1>=14.04.0)
include_directories(${DBUSTEST_INCLUDE_DIRS})

pkg_check_modules(GIO2 REQUIRED gio-2.0 gio-unix-2.0)
include_directories(${GIO2_INCLUDE_DIRS})

# Google Test

include_directories(${GTEST_INCLUDE_DIR})

#add_library (gtest2 STATIC
#             ${GTEST_SOURCE_DIR}/gtest-all.cc
#             ${GTEST_SOURCE_DIR}/gtest_main.cc)

# libUAL Test
include_directories(${UAL_INCLUDE_DIRS})
include_directories(${PROP-CPP_INCLUDE_DIRS})

add_definitions ( -DSPEW_UTILITY="${CMAKE_CURRENT_BINARY_DIR}/data-spew" )
add_definitions ( -DSESSION_TEMP_FILE="${CMAKE_CURRENT_BINARY_DIR}/libual-test-session-start-temp" )
add_definitions ( -DSOCKET_DEMANGLER="${CMAKE_BINARY_DIR}/socket-demangler" )
add_definitions ( -DSOCKET_DEMANGLER_INSTALL="${pkglibexecdir}/socket-demangler" )
add_definitions ( -DSOCKET_TOOL="${CMAKE_CURRENT_BINARY_DIR}/socket-tool" )

add_executable (helpers-test
    helpers-test.cc
    mir-mock.cpp
)

set_target_properties(helpers-test PROPERTIES COMPILE_FLAGS -fPIC)
set_target_properties(helpers-test PROPERTIES AUTOMOC TRUE)
qt5_use_modules(helpers-test Core DBus Test)

target_link_libraries(helpers-test
                        gtest
                        backup-helper
                        ${GTEST_LIBS}
                        ${LIBUPSTART_LIBRARIES}
                        ${DBUSTEST_LIBRARIES}
                        ${UAL_LIBRARIES}
                        ${GIO2_LIBRARIES})

add_test (NAME helpers-test COMMAND helpers-test)
