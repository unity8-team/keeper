#
# the builtin helpers
#

set(
  FOLDER_BACKUP
  folder-backup.sh
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_BACKUP}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_BACKUP}
)

install(
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_BACKUP}
  DESTINATION
    ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)

set(
  FOLDER_RESTORE
  folder-restore.sh
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_RESTORE}.in
  ${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_RESTORE}
)

install(
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_RESTORE}
  DESTINATION
    ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)

#
# the library
#

add_definitions(
  -DHELPER_REGISTRY_FILENAME="${HELPER_REGISTRY_FILENAME}"
  -DPROJECT_NAME="${PROJECT_NAME}"
)

include(FindPkgConfig)
pkg_check_modules(BACKUP_HELPER_DEPENDENCIES REQUIRED
  ubuntu-app-launch-3
  properties-cpp
)

include_directories(
  SYSTEM
  ${BACKUP_HELPER_DEPENDENCIES_INCLUDE_DIRS}
)

set(
  HELPER_LIB
  backup-helper
)

add_library(
  ${HELPER_LIB}
  STATIC
  backup-helper.cpp
  restore-helper.cpp
  data-dir-registry.cpp
  helper.cpp
  metadata.cpp
  ${CMAKE_SOURCE_DIR}/include/helper/backup-helper.h
  ${CMAKE_SOURCE_DIR}/include/helper/restore-helper.h
  ${CMAKE_SOURCE_DIR}/include/helper/data-dir-registry.h
  ${CMAKE_SOURCE_DIR}/include/helper/helper.h
  ${CMAKE_SOURCE_DIR}/include/helper/registry.h
  ${CMAKE_SOURCE_DIR}/include/helper/metadata.h
)

set_target_properties(
  ${HELPER_LIB}
  PROPERTIES
  AUTOMOC TRUE
)

target_link_libraries(
  ${HELPER_LIB}
  util
  storage-framework
  ${BACKUP_HELPER_DEPENDENCIES_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Network
)

install(
  PROGRAMS
    ${CMAKE_CURRENT_SOURCE_DIR}/exec-tool
  DESTINATION
    ${CMAKE_INSTALL_LIBEXECDIR}/ubuntu-app-launch/backup-helper
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${HELPER_LIB}
  PARENT_SCOPE
)
