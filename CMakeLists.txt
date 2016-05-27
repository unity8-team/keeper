
# Default install location. Must be set here, before setting the project.
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "" FORCE)
  set(LOCAL_INSTALL "ON")
endif()

project(keeper C CXX)
cmake_minimum_required(VERSION 3.5.1)

##
##  Build Type
##

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower) # Build types should always be lower case

set(ACCEPTED_BUILD_TYPES "" none release debug relwithdebinfo coverage)
list(FIND ACCEPTED_BUILD_TYPES "${cmake_build_type_lower}" IS_BUILD_TYPE_ACCEPTED)
if (${IS_BUILD_TYPE_ACCEPTED} EQUAL -1)
    message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}\nValid types are: ${ACCEPTED_BUILD_TYPES}")
endif()

# By default, for release builds, warnings become hard errors.
if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
    option(Werror "Treat warnings as errors" ON)
else()
    option(Werror "Treat warnings as errors" OFF)
endif()


##
##  GNU standard paths
##

include(GNUInstallDirs)
if(EXISTS "/etc/debian_version") # Workaround for libexecdir on debian
    set(CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()
set(CMAKE_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
set(CMAKE_INSTALL_FULL_PKGLIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")


##
##  Toolkits
##

find_package(Threads REQUIRED)

set(CMAKE_AUTOMOC ON)

add_definitions(
    -DQT_NO_KEYWORDS=1
)

find_package(Qt5Core REQUIRED)
include_directories(SYSTEM ${Qt5Core_INCLUDE_DIRS})

find_package(Qt5DBus COMPONENTS Qt5DBusMacros REQUIRED)
include_directories(SYSTEM ${Qt5DBus_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)

pkg_check_modules(SERVICE_DEPS REQUIRED
    libarchive>=3.1.2
)
include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})


##
##  Global build options
##

set(C_OPTIONS
  -fPIC
  -fvisibility=hidden
  -g
)
set(CXX_OPTIONS
  ${C_OPTIONS}
  -std=c++14
)

set(CLANG_C_WARNINGS
  -Weverything
  -Wno-padded
)
set(CLANG_CXX_WARNINGS
  ${CLANG_C_WARNINGS}
  -Wno-c++98-compat
)

set(GNU_C_WARNINGS
  -Wall
  -pedantic
  -Wextra
  -Wcast-align
  -Wcast-qual
  -Wformat
  -Wredundant-decls
)
set(GNU_CXX_WARNINGS
  ${GNU_C_WARNINGS}
  -Wnon-virtual-dtor
  -Wctor-dtor-privacy
  -Wold-style-cast
)

add_compile_options(
  "$<$<COMPILE_LANGUAGE:C>:${C_OPTIONS}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${CXX_OPTIONS}>"
  "$<$<AND:$<COMPILE_LANGUAGE:C>,$<CXX_COMPILER_ID:GNU>>:${GNU_C_WARNINGS}>"
  "$<$<AND:$<COMPILE_LANGUAGE:C>,$<CXX_COMPILER_ID:Clang>>:${CLANG_C_WARNINGS}>"
  "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:${GNU_CXX_WARNINGS}>"
  "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:${CLANG_CXX_WARNINGS}>"
)

# If warnings are errors, don't error on deprecated declarations.
if (${Werror})
  add_compile_options(-Werror)
  if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
    add_compile_options(-Wno-error=deprecated-declarations)
  endif()
endif()

add_definitions(
  -DG_LOG_DOMAIN="${PACKAGE}"
  -DGETTEXT_PACKAGE="${PACKAGE}"
  -DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATADIR}/locale"
)

##
##
##

# Flags for address and undefined behavior sanitizer
set(SANITIZER "" CACHE STRING "Build with -fsanitize=<value> (legal values: address, ub)")

if ("${SANITIZER}" STREQUAL "")
    # Do nothing
elseif (${SANITIZER} STREQUAL "ub")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -g -fsanitize=float-divide-by-zero")
elseif (${SANITIZER} STREQUAL "address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
else()
    message(FATAL_ERROR "Invalid SANITIZER setting: ${SANITIZER}")
endif()


# Some tests are slow, so make it possible not to run them
# during day-to-day development.
option(slowtests "Run slow tests" ON)
if (${slowtests})
    add_definitions(-DSLOW_TESTS=1)
else()
    add_definitions(-DSLOW_TESTS=0)
endif()


# Definitions for testing with valgrind.
# Tests in CTestCustom.cmake are skipped for valgrind
configure_file(
  CTestCustom.cmake.in
  CTestCustom.cmake
)

find_program(MEMORYCHECK_COMMAND NAMES valgrind)
if (MEMORYCHECK_COMMAND)
    set(MEMORYCHECK_COMMAND_OPTIONS
        "--suppressions=${CMAKE_SOURCE_DIR}/valgrind-suppress --errors-for-leak-kinds=definite --show-leak-kinds=definite --leak-check=full --num-callers=50 --error-exitcode=3"
    )
    add_custom_target(valgrind DEPENDS NightlyMemCheck)
else()
    message(WARNING "Cannot find valgrind: valgrind target will not be available")
endif()

include(CTest)
enable_testing()

include(EnableCoverageReport)

add_subdirectory(src)
add_subdirectory(tests)

enable_coverage_report(
    TARGETS
    FILTER
        ${CMAKE_SOURCE_DIR}/tests/*
        ${CMAKE_BINARY_DIR}/*
    TESTS
        ${UNIT_TEST_TARGETS}
)
