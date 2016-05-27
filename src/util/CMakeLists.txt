include_directories("${CMAKE_SOURCE_DIR}/src/")

set(UTIL_SOURCES
    dbus-utils.cpp
    logging.cpp
    unix-signal-handler.cpp
)

add_library(util STATIC ${UTIL_SOURCES})
