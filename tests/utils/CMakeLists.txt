
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(
    test-utils
    STATIC
    main.cpp
)

qt5_use_modules(
    test-utils
    Core
)
