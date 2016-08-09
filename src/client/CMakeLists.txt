set(
  CLIENT_LIB
  keeper-client
)

set(
  CLIENT_HEADERS
  ${CMAKE_SOURCE_DIR}/include/client/client.h
)

add_library(
  ${CLIENT_LIB} SHARED
  client.cpp
  ${CLIENT_HEADERS}
)

set_target_properties(
  ${CLIENT_LIB}
  PROPERTIES
  AUTOMOC TRUE
)

target_link_libraries(
  ${CLIENT_LIB}
  qdbus-stubs
  Qt5::Core
  Qt5::DBus
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${CLIENT_LIB}
  PARENT_SCOPE
)

install(
  FILES ${CLIENT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/keeper
)

install(
  TARGETS ${CLIENT_LIB}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/keeper
)
