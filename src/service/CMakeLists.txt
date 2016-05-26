set(SERVICE_EXEC "keeper-service")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(SERVICE_SOURCES
  main.cpp
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
  util
  ${SERVICE_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Threads::Threads
)

install(
  TARGETS
    ${SERVICE_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
