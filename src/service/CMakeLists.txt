set(SERVICE_EXEC "keeper-service")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(SERVICE_SOURCES
  backup-choices.cpp
  keeper.cpp
  keeper-user.cpp
  main.cpp
  metadata.cpp
)

add_executable(
  ${SERVICE_EXEC}
  ${SERVICE_SOURCES}
)

link_directories(
  ${SERVICE_DEPS_LIBRARY_DIRS}
)

target_link_libraries(
  ${SERVICE_EXEC}
  storage
  util
  qdbus-stubs
  storage-framework
  backup-helper
  ${SERVICE_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

install(
  TARGETS
    ${SERVICE_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
