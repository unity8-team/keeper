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
  ${SERVICE_DEPS_LDFLAGS}
  Qt5::Core
  Qt5::Network
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${LIB_NAME}
  PARENT_SCOPE
)
