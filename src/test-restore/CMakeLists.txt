set(TEST_RESTORE_EXEC "test-restore-socket")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(TEST_RESTORE_SOURCES
  main.cpp
  test-restore-socket.cpp
)

add_executable(
  ${TEST_RESTORE_EXEC}
  ${TEST_RESTORE_SOURCES}
)

link_directories(
  ${SERVICE_DEPS_LIBRARY_DIRS}
)

set(
  RESTORE_STATIC_LIBS
  storage-framework
  util
#  qdbus-stubs
)

target_link_libraries(
  ${TEST_RESTORE_EXEC}
  ${RESTORE_STATIC_LIBS}
  ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

install(
  TARGETS
    ${TEST_RESTORE_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
