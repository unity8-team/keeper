set(
  LIB_NAME
  storage-framework
)

add_library(
  ${LIB_NAME}
  STATIC
  storage_framework_client.cpp
  storage_framework_client.h
)

set_target_properties(
  ${LIB_NAME}
  PROPERTIES
  AUTOMOC TRUE
)
target_link_libraries(
  ${LIB_NAME}
  -lstorage-framework-qt-local-client
  Qt5::Core
  Qt5::Network
)

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${LIB_NAME}
  PARENT_SCOPE
)
