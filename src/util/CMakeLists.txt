set(
  LIB_NAME
  util
)

add_definitions(
  -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_library(
  ${LIB_NAME}
  STATIC
  connection-helper.h
  dbus-utils.cpp
  logging.cpp
  unix-signal-handler.cpp
)

set_target_properties(
  ${LIB_NAME}
  PROPERTIES
  AUTOMOC TRUE
)

target_link_libraries(
  ${LIB_NAME}
  Qt5::Core
)
