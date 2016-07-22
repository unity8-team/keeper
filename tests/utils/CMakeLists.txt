include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(
  LIB_NAME
  test-utils
)

add_library(
  ${LIB_NAME}
  STATIC
  main.cpp
  file-utils.cpp
  xdg-user-dirs-sandbox.cpp
)

target_link_libraries(
  ${LIB_NAME}
  backup-helper
  Qt5::Core
)
