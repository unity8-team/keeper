set(UTIL_SOURCES
    dbus-utils.cpp
    logging.cpp
    unix-signal-handler.cpp
)

add_definitions(-DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_library(util STATIC ${UTIL_SOURCES})
