set(SERVICE_LIB "keeperservice")
set(SERVICE_EXEC "keeper-service")
set(SERVICE_LOCAL_EXEC "keeper-service-local")

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
  manifest.cpp
  metadata-provider.h
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

add_executable(
  ${SERVICE_LOCAL_EXEC}
  ${SERVICE_EXEC_SOURCES}
)

link_directories(
  ${SERVICE_DEPS_LIBRARY_DIRS}
)

set(
  SERVICE_STATIC_LIBS
  backup-helper
  storage-framework
  util
  qdbus-stubs
)

target_link_libraries(
  ${SERVICE_EXEC}
  ${SERVICE_LIB}
  ${SERVICE_STATIC_LIBS}
  ${SERVICE_DEPS_LIBRARIES}
  ${SERVICE_PRODUCTION_SF_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

target_link_libraries(
  ${SERVICE_LOCAL_EXEC}
  ${SERVICE_LIB}
  ${SERVICE_STATIC_LIBS}
  ${SERVICE_DEPS_LIBRARIES}
  ${SERVICE_DEVEL_SF_DEPS_LIBRARIES}
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
