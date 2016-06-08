
include(FindPkgConfig)
pkg_check_modules(UAL REQUIRED ubuntu-app-launch-2)
pkg_check_modules(PROP-CPP REQUIRED properties-cpp)

include_directories(${UAL_INCLUDE_DIRS})
include_directories(${PROP-CPP_INCLUDE_DIRS})

add_library(
  backup-helper
  STATIC
  backup-helper.cpp
  internal/backup-helper-impl.cpp
  ${CMAKE_SOURCE_DIR}/include/helper/backup-helper.h
  internal/backup-helper-impl.h
)

qt5_use_modules(backup-helper Core DBus)
set_target_properties(backup-helper PROPERTIES AUTOMOC TRUE)

target_link_libraries(backup-helper
                      ${UAL_LIBRARIES})
