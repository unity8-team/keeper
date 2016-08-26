add_subdirectory(qml-plugin)

set(
  CLIENT_HEADERS
  ${CMAKE_SOURCE_DIR}/include/client/client.h
)

add_library(
  ${KEEPER_CLIENT_LIB} SHARED
  client.cpp
  ${CLIENT_HEADERS}
)

target_link_libraries(
  ${KEEPER_CLIENT_LIB}
  qdbus-stubs
  Qt5::Core
  Qt5::DBus
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${KEEPER_CLIENT_LIB}
  PARENT_SCOPE
)

install(
  FILES ${CLIENT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/keeper
)

install(
  TARGETS ${KEEPER_CLIENT_LIB}
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
