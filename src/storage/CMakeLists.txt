set(STORAGE_SOURCES
    storage-factory.cpp
    internal/
)

add_library(storage STATIC ${STORAGE_SOURCES})
