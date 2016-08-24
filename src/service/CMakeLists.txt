set(SERVICE_LIB "keeperservice")
set(SERVICE_EXEC "keeper-service")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(SERVICE_LIB_SOURCES
  backup-choices.cpp
  keeper.cpp
  keeper-user.cpp
  keeper-helper.cpp
  restore-choices.cpp
  task-manager.cpp
  keeper-task.cpp
  keeper-task-backup.cpp
)
add_library(
  ${SERVICE_LIB}
  STATIC
  ${SERVICE_LIB_SOURCES}
)

set(SERVICE_EXEC_SOURCES
  main.cpp
)

add_executable(
  ${SERVICE_EXEC}
  ${SERVICE_EXEC_SOURCES}
)

link_directories(
  ${SERVICE_DEPS_LIBRARY_DIRS}
)

target_link_libraries(
  ${SERVICE_EXEC}
  ${SERVICE_LIB}
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

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${SERVICE_LIB}
  ${SERVICE_EXEC}
  PARENT_SCOPE
)
