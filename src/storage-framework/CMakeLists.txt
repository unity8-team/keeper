
add_library(
  storage-framework
  STATIC
  storage_framework_client.cpp
  storage_framework_client.h
)

qt5_use_modules(storage-framework Core Network)
set_target_properties(backup-helper PROPERTIES AUTOMOC TRUE)

target_link_libraries(storage-framework
                      ${STORAGE_FW_DEPS_LIBRARIES})
