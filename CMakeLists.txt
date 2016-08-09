
# Default install location. Must be set here, before setting the project.
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "" FORCE)
  set(LOCAL_INSTALL "ON")
endif()

project(keeper C CXX)
cmake_minimum_required(VERSION 3.0.2)

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
    -DQT_MESSAGELOGCONTEXT
)

find_package(Qt5Core REQUIRED)
include_directories(SYSTEM ${Qt5Core_INCLUDE_DIRS})

find_package(Qt5Concurrent REQUIRED)
include_directories(SYSTEM ${Qt5Concurrent_INCLUDE_DIRS})

find_package(Qt5DBus COMPONENTS Qt5DBusMacros REQUIRED)
include_directories(SYSTEM ${Qt5DBus_INCLUDE_DIRS})

find_package(Qt5Network REQUIRED)
include_directories(SYSTEM ${Qt5Network_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)

pkg_check_modules(SERVICE_DEPS REQUIRED
    click-0.4>=0.4.30
    glib-2.0
    gobject-2.0
    json-glib-1.0
    libarchive>=3.1.2
    storage-framework-qt-local-client-1
    uuid>=2.25
)
include_directories(SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)


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
)

# Throw nearly everything into the compiler warnings.
# This list isn't set in stone, feel free to disable items when sensible.

set(C_WARNINGS)
set(CXX_WARNINGS)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

  LIST(APPEND CXX_OPTIONS -std=c++14)

  LIST(APPEND C_WARNINGS
    -Weverything
    -Wno-padded
  )
  LIST(APPEND CXX_WARNINGS
    -Wno-c++98-compat
  )

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

  if(NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "5.0.0")
    LIST(APPEND CXX_OPTIONS -std=c++14)
  elseif(NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.8.0")
    LIST(APPEND CXX_OPTIONS -std=c++11)
  else()
    LIST(APPEND CXX_OPTIONS -std=c++0x -Dfinal= -Doverride=)
  endif()

  LIST(APPEND C_WARNINGS
    -Wall
    -pedantic
    -Wextra
    -Wcast-align
    -Wcast-qual
    -Wconditionally-supported
    -Wconversion
    -Wdate-time
    -Wdouble-promotion
    -Wfloat-equal
    -Wformat=2
    -Winline
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Woverlength-strings
    -Wpacked
    -Wredundant-decls
    -Wsuggest-attribute=const
    -Wsuggest-attribute=format
    -Wsuggest-attribute=noreturn
    -Wswitch
    -Wtrampolines
    -Wundef
    -Wunused
    -Wwrite-strings
  )

  LIST(APPEND CXX_WARNINGS
    -Wctor-dtor-privacy
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wsign-promo
    -Wstrict-null-sentinel
  )

  if(NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "5.0.0")

    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57709
    LIST(APPEND C_WARNINGS
      -Wshadow
    )

    LIST(APPEND CXX_WARNINGS
      -Wsized-deallocation
      -Wsuggest-attribute=pure
      -Wsuggest-final-methods
      -Wsuggest-final-types
      -Wsuggest-override
    )

  endif()

endif()

add_compile_options(
  ${C_OPTIONS}
  ${C_WARNINGS}
  ${CXX_OPTIONS}
  ${CXX_WARNINGS}
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

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(tests)

enable_coverage_report(
    TARGETS
        ${COVERAGE_REPORT_TARGETS}
    FILTER
        ${CMAKE_SOURCE_DIR}/tests/*
        ${CMAKE_BINARY_DIR}/*
    TESTS
        ${COVERAGE_TEST_TARGETS}
)
