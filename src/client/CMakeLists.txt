include(FindPkgConfig)
pkg_check_modules(CLIENT_DEPENDENCIES REQUIRED
)

include_directories(
  SYSTEM
  ${CLIENT_DEPENDENCIES_INCLUDE_DIRS}
)

set(
  CLIENT_LIB
  backup-client
)

add_library(
  ${CLIENT_LIB} SHARED
  client.cpp
  ${CMAKE_SOURCE_DIR}/include/client/client.h
)

set_target_properties(
  ${CLIENT_LIB}
  PROPERTIES
  AUTOMOC TRUE
)

target_link_libraries(
  ${CLIENT_LIB}
  ${CLIENT_DEPENDENCIES_LIBRARIES}
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${CLIENT_LIB}
  PARENT_SCOPE
)
