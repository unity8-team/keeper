include(FindPkgConfig)
pkg_check_modules(BACKUP_HELPER_DEPENDENCIES REQUIRED
  ubuntu-app-launch-2
  properties-cpp
)

include_directories(
  SYSTEM
  ${BACKUP_HELPER_DEPENDENCIES_INCLUDE_DIRS}
)

add_library(
  backup-helper
  STATIC
  helper.cpp
  backup-helper.cpp
  ${CMAKE_SOURCE_DIR}/include/helper/helper.h
  ${CMAKE_SOURCE_DIR}/include/helper/backup-helper.h
)

set_target_properties(
  backup-helper
  PROPERTIES
  AUTOMOC TRUE
)

target_link_libraries(
  backup-helper
  ${BACKUP_HELPER_DEPENDENCIES_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Network
)

install(
  FILES ${CMAKE_SOURCE_DIR}/src/helper/exec-tool
  DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/ubuntu-app-launch/backup-helper
)
