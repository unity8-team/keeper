
#
# storage-framework-folders-test
#

set(
  STORAGE_FRAMEWORK_FOLDERS_TEST
  storage-framework-folders-test
)

add_executable(
  ${STORAGE_FRAMEWORK_FOLDERS_TEST}
  folders-test.cpp
)

set_target_properties(
  ${STORAGE_FRAMEWORK_FOLDERS_TEST}
  PROPERTIES
  COMPILE_FLAGS -fPIC
  AUTOMOC TRUE
)

target_link_libraries(
  ${STORAGE_FRAMEWORK_FOLDERS_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${STORAGE_FRAMEWORK_FOLDERS_TEST}
  COMMAND ${STORAGE_FRAMEWORK_FOLDERS_TEST}
)

#
# create-uploader-test
#

set(
  STORAGE_FRAMEWORK_UPLOADER_TEST
  create-uploader-test
)

add_executable(
  ${STORAGE_FRAMEWORK_UPLOADER_TEST}
  create-uploader-test.cpp
)

target_link_libraries(
  ${STORAGE_FRAMEWORK_UPLOADER_TEST}
  ${UNIT_TEST_LIBRARIES}
  Qt5::Core
  Qt5::DBus
  Qt5::Test
)

add_test(
  NAME ${STORAGE_FRAMEWORK_UPLOADER_TEST}
  COMMAND ${STORAGE_FRAMEWORK_UPLOADER_TEST}
)

#
#
#

set(
  COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  ${STORAGE_FRAMEWORK_UPLOADER_TEST}
  ${STORAGE_FRAMEWORK_FOLDERS_TEST}
  PARENT_SCOPE
)
