include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(STORAGE_SOURCES
    storage.cpp
    storage-factory.cpp
)

add_library(storage STATIC ${STORAGE_SOURCES})
