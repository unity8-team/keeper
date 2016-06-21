include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

add_executable(
  simple-helper
  main.cpp
)

target_link_libraries(
  simple-helper
  qdbus-stubs
  Qt5::Core
  Qt5::DBus
  Qt5::Network
)

install(
  TARGETS
    simple-helper
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)