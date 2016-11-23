set(OWNCLOUD_EXEC "owncloud-test")

include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src")

set(OWNCLOUD_EXEC_SOURCES
  main.cpp
  owncloud-test.cpp
  owncloud-test.h
)

add_executable(
  ${OWNCLOUD_EXEC}
  ${OWNCLOUD_EXEC_SOURCES}
)

target_link_libraries(
  ${OWNCLOUD_EXEC}
  #backup-helper
  storage-framework
  util
  qdbus-stubs
  ${SERVICE_PRODUCTION_SF_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

