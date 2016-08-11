add_test(cppcheck cppcheck --enable=all -USCHEMA_DIR --error-exitcode=2 --inline-suppr -I${CMAKE_SOURCE_DIR}/src -I${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
add_test(flake8 python3 -m flake8 ${SERVICE_TEMPLATE_FILE})
add_formatcode_test(${PROJECT_SOURCES} STYLE_NAME unity-api)

add_subdirectory(copyright)
add_subdirectory(whitespace)

# don't include any of the 'surface' tests in memcheck
file(APPEND
  ${CMAKE_BINARY_DIR}/CTestCustom.cmake
  "LIST(APPEND CTEST_CUSTOM_MEMCHECK_IGNORE cppcheck flake8 copyright whitespace)\n"
)

# if cppcheck is too old, don't fail the tests on it
execute_process(
  COMMAND cppcheck --version
  OUTPUT_VARIABLE CPPCHECK_VERSION_STR
)
string(
  REGEX MATCH "([0-9]*[.])?[0-9]+"
  CPPCHECK_VERSION
  ${CPPCHECK_VERSION_STR}
)
if(${CPPCHECK_VERSION} LESS "1.72")
  file(
    APPEND
    ${CMAKE_BINARY_DIR}/CTestCustom.cmake
    "LIST(APPEND CTEST_CUSTOM_TESTS_IGNORE cppcheck)\n"
  )
endif()
