
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(
    LIB_NAME
    test-utils
)

add_library(
    ${LIB_NAME}
    STATIC
    main.cpp
    xdg-user-dirs-sandbox.cpp
)

qt5_use_modules(
    ${LIB_NAME}
    Core
)
