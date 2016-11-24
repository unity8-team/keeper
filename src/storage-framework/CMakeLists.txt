set(
  LIB_NAME
  storage-framework
)

add_library(
  ${LIB_NAME}
  STATIC
  storage_framework_client.cpp
  storage_framework_client.h
  uploader.h
  sf-uploader.cpp
  sf-uploader.h
  downloader.h
  sf-downloader.cpp
  sf-downloader.h
)

set_target_properties(
  ${LIB_NAME}
  PROPERTIES
  AUTOMOC TRUE
)
target_link_libraries(
  ${LIB_NAME}
  Qt5::Core
  Qt5::Network
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${LIB_NAME}
  PARENT_SCOPE
)
