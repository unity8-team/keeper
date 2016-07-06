set(APP_LIB "keepertar")
set(APP_EXEC "keeper-tar-creator")

set(APP_LIB_SOURCES
    tar-creator.cpp
)
add_library(
    ${APP_LIB}
    STATIC
    ${APP_LIB_SOURCES}
)

set(APP_EXEC_SOURCES
    main.cpp
)

add_executable(
    ${APP_EXEC}
    ${APP_EXEC_SOURCES}
)

link_directories(
    ${SERVICE_DEPS_LIBRARY_DIRS}
)

target_link_libraries(
    ${APP_EXEC}
    ${APP_LIB}
    qdbus-stubs
    storage-framework
    backup-helper
    ${SERVICE_DEPS_LIBRARIES}
    Qt5::Core
    Qt5::DBus
)

install(
    TARGETS
        ${APP_EXEC}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
