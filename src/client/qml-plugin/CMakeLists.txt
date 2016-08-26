set(
  QML_CLIENT_LIB
  keeper-qml
)

add_definitions(-DKEEPER_MAJOR=${KEEPER_MAJOR})
add_definitions(-DKEEPER_MINOR=${KEEPER_MINOR})

include(QmlPlugins)

find_package(Qt5Quick REQUIRED)

include_directories(
  "${CMAKE_SOURCE_DIR}/include/client"
)

set(KEEPER_QML_SRC
  plugin.cpp
)

add_library(${QML_CLIENT_LIB} SHARED
  ${KEEPER_QML_SRC}
)

target_link_libraries(
  ${QML_CLIENT_LIB}
  ${CLIENT_LIB}
)

qt5_use_modules(
  ${QML_CLIENT_LIB}
  Qml
)

add_qmlplugin(
  Ubuntu.Keeper
  ${KEEPER_MAJOR}.${KEEPER_MINOR}
  Ubuntu/Keeper
  TARGET_PREFIX Keeper
  TARGETS ${QML_CLIENT_LIB}
)
