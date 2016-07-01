set(APP_LIB "keepertarcreator")
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
    ${SERVICE_DEPS_LIBRARIES}
    Qt5::Core
)

install(
    TARGETS
        ${APP_EXEC}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
