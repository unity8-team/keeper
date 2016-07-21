add_test(cppcheck cppcheck --enable=all -USCHEMA_DIR --error-exitcode=2 --inline-suppr -I${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
add_test(flake8 flake8 ${SERVICE_TEMPLATE_FILE})

add_subdirectory(copyright)
add_subdirectory(whitespace)
