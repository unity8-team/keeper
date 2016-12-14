set(LIB_NAME "keepertar")
set(KEEPER_TAR_APP_NAME "keeper-tar")
set(KEEPER_UNTAR_APP_NAME "keeper-untar")

##
##  library
##

set(LIB_SOURCES
  tar-creator.cpp
  untar.cpp
)
add_library(
  ${LIB_NAME}
  STATIC
  ${LIB_SOURCES}
)

link_directories(
  ${SERVICE_DEPS_LIBRARY_DIRS}
)

##
##  tar creator
##

set_property(
  SOURCE tar-creator-main.cpp
  PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS APP_NAME=\"${KEEPER_TAR_APP_NAME}\"
)

set(KEEPER_UNTAR_APP_SOURCES
  untar-main.cpp
)

add_executable(
  ${KEEPER_UNTAR_APP_NAME}
  ${KEEPER_UNTAR_APP_SOURCES}
)

target_link_libraries(
  ${KEEPER_UNTAR_APP_NAME}
  ${LIB_NAME}
  qdbus-stubs
  storage-framework
  backup-helper
  ${SERVICE_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

##
##  untar
##

set_property(
  SOURCE untar-main.cpp
  PROPERTIES APPEND_STRING PROPERTY COMPILE_DEFINITIONS APP_NAME=\"${KEEPER_UNTAR_APP_NAME}\"
)

set(KEEPER_TAR_APP_SOURCES
  tar-creator-main.cpp
)

add_executable(
  ${KEEPER_TAR_APP_NAME}
  ${KEEPER_TAR_APP_SOURCES}
)

target_link_libraries(
  ${KEEPER_TAR_APP_NAME}
  ${LIB_NAME}
  qdbus-stubs
  storage-framework
  backup-helper
  ${SERVICE_DEPS_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

##
##
##

install(
  TARGETS
    ${KEEPER_TAR_APP_NAME}
    ${KEEPER_UNTAR_APP_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)

set(
  COVERAGE_REPORT_TARGETS
  ${COVERAGE_REPORT_TARGETS}
  ${LIB_NAME}
  ${KEEPER_TAR_APP_NAME}
  ${KEEPER_UNTAR_APP_NAME}
  PARENT_SCOPE
)
