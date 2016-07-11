set(LIB_NAME "keepertar")
set(APP_NAME "keeper-tar-create")

set(LIB_SOURCES
    tar-creator.cpp
)
add_library(
    ${LIB_NAME}
    STATIC
    ${LIB_SOURCES}
)

set_property(
    SOURCE main.cpp
    PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS APP_NAME=\"${APP_NAME}\"
)

set(APP_SOURCES
    main.cpp
)

add_executable(
    ${APP_NAME}
    ${APP_SOURCES}
)

link_directories(
    ${SERVICE_DEPS_LIBRARY_DIRS}
)

target_link_libraries(
    ${APP_NAME}
    ${LIB_NAME}
    qdbus-stubs
    storage-framework
    backup-helper
    ${SERVICE_DEPS_LIBRARIES}
    Qt5::Core
    Qt5::DBus
)

install(
    TARGETS
        ${APP_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
