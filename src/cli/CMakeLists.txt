set(CLI_EXEC "keeper")

include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(CLI_SOURCES
  main.cpp
)

add_executable(
  ${CLI_EXEC}
  ${CLI_SOURCES}
)

target_link_libraries(
  ${CLI_EXEC}
  storage
  util
  qdbus-stubs
  Qt5::Core
  Qt5::DBus
)

install(
  TARGETS
    ${CLI_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
