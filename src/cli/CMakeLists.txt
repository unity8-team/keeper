set(CLI_EXEC "keeper")

include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(CLI_SOURCES
  main.cpp
  command-line.cpp
  command-line-client.cpp
  command-line-client-view.cpp
)

add_executable(
  ${CLI_EXEC}
  ${CLI_SOURCES}
)

target_link_libraries(
  ${CLI_EXEC}
  backup-helper
  storage-framework
  util
  qdbus-stubs
  ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
  ${KEEPER_CLIENT_LIB}
  Qt5::Core
  Qt5::DBus
)

install(
  TARGETS
    ${CLI_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${CLI_EXEC}
  PARENT_SCOPE
)
