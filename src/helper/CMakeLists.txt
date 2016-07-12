
include(FindPkgConfig)
pkg_check_modules(UAL REQUIRED ubuntu-app-launch-2)
include_directories(SYSTEM ${UAL_INCLUDE_DIRS})
pkg_check_modules(PROP-CPP REQUIRED properties-cpp)
include_directories(SYSTEM ${PROP-CPP_INCLUDE_DIRS})

add_library(
  backup-helper
  STATIC
  backup-helper.cpp
  ${CMAKE_SOURCE_DIR}/include/helper/backup-helper.h
)

qt5_use_modules(backup-helper Core DBus Network)
set_target_properties(backup-helper PROPERTIES AUTOMOC TRUE)

target_link_libraries(backup-helper
                      ${UAL_LIBRARIES})

install(
  FILES ${CMAKE_SOURCE_DIR}/src/helper/exec-tool
  DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/ubuntu-app-launch/backup-helper
)
