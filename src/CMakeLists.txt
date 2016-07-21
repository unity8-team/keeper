include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/src"
)

add_subdirectory(cli)
add_subdirectory(helper)
add_subdirectory(qdbus-stubs)
add_subdirectory(service)
add_subdirectory(simple-helper)
add_subdirectory(storage-framework)
add_subdirectory(tar)
add_subdirectory(util)
