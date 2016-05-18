set(BACKUP_EXEC "keeper-backup")

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

set(BACKUP_SOURCES
  main.cpp
)

add_executable(
  ${BACKUP_EXEC}
  ${BACKUP_SOURCES}
)

target_link_libraries(
  ${BACKUP_EXEC}
  Qt5::Core
  Qt5::DBus
  Threads::Threads
)

install(
  TARGETS
    ${BACKUP_EXEC}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
