set(
  CLIENT_LIB
  keeper-client-${KEEPER_MAJOR}.${KEEPER_MINOR}
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
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

configure_file(
    keeper-client.pc.in
    keeper-client.pc
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/keeper-client.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
