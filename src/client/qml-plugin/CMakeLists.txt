set(
  QML_KEEPER_CLIENT_LIB
  keeper-qml
)

add_definitions(-DKEEPER_MAJOR=${KEEPER_MAJOR})
add_definitions(-DKEEPER_MINOR=${KEEPER_MINOR})

find_package(QmlPlugins)

find_package(Qt5Quick REQUIRED)

include_directories(
  "${CMAKE_SOURCE_DIR}/include/client"
)

set(KEEPER_QML_SRC
  plugin.cpp
)

add_library(
  ${QML_KEEPER_CLIENT_LIB} SHARED
  ${KEEPER_QML_SRC}
)

target_link_libraries(
  ${QML_KEEPER_CLIENT_LIB}
  ${KEEPER_CLIENT_LIB}
)

qt5_use_modules(
  ${QML_KEEPER_CLIENT_LIB}
  Qml
)

add_qmlplugin(
  Ubuntu.Keeper
  ${KEEPER_MAJOR}.${KEEPER_MINOR}
  Ubuntu/Keeper
  TARGET_PREFIX Keeper
  TARGETS ${QML_KEEPER_CLIENT_LIB}
)
