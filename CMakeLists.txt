cmake_minimum_required(VERSION 3.20)

project(mcprotocol_serial_cpp VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MCPROTOCOL_BUILD_EXAMPLES "Build compile-checked example programs" ON)
if(WIN32)
  set(MCPROTOCOL_DOXYGEN_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_doxygen.bat)
else()
  set(MCPROTOCOL_DOXYGEN_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_doxygen.sh)
endif()
set(MCPROTOCOL_MARKDOWN_LINK_CHECKER ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_markdown_links.py)

set(MCPROTOCOL_SERIAL_PLATFORM_SOURCES)
if(WIN32)
  list(APPEND MCPROTOCOL_SERIAL_PLATFORM_SOURCES
    src/win32_serial.cpp
  )
else()
  list(APPEND MCPROTOCOL_SERIAL_PLATFORM_SOURCES
    src/posix_serial.cpp
  )
endif()

add_library(mcprotocol_serial
  src/client.cpp
  src/codec.cpp
  src/host_sync.cpp
  ${MCPROTOCOL_SERIAL_PLATFORM_SOURCES}
)

target_include_directories(mcprotocol_serial
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(mcprotocol_serial
    PUBLIC
      -fno-exceptions
      -fno-rtti
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )
elseif(MSVC)
  target_compile_options(mcprotocol_serial
    PUBLIC
      /EHs-c-
      /GR-
    PRIVATE
      /W4
  )
endif()

include(CTest)
find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_Interpreter_FOUND)
  set(MCPROTOCOL_PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
else()
  set(MCPROTOCOL_PYTHON_EXECUTABLE python3)
endif()

if(BUILD_TESTING)
  add_executable(codec_tests
    tests/codec_tests.cpp
  )

  target_link_libraries(codec_tests PRIVATE mcprotocol_serial)

  add_test(NAME codec_tests COMMAND codec_tests)
endif()

add_executable(mcprotocol_cli
  tools/mcprotocol_cli.cpp
)

target_link_libraries(mcprotocol_cli PRIVATE mcprotocol_serial)

if(MCPROTOCOL_BUILD_EXAMPLES)
  add_executable(mcprotocol_example_mcu_async
    examples/mcu_async_batch_read.cpp
  )

  target_link_libraries(mcprotocol_example_mcu_async PRIVATE mcprotocol_serial)

  add_executable(mcprotocol_example_host_sync
    examples/host_sync_quickstart.cpp
  )

  target_link_libraries(mcprotocol_example_host_sync PRIVATE mcprotocol_serial)
endif()

set(MCPROTOCOL_DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(MCPROTOCOL_DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${MCPROTOCOL_DOXYFILE_IN} ${MCPROTOCOL_DOXYFILE_OUT} @ONLY)

add_custom_target(docs
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs/api
  COMMAND ${MCPROTOCOL_DOXYGEN_RUNNER} ${MCPROTOCOL_DOXYFILE_OUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)

add_custom_target(check-markdown-links
  COMMAND ${MCPROTOCOL_PYTHON_EXECUTABLE} ${MCPROTOCOL_MARKDOWN_LINK_CHECKER}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking Markdown links"
  VERBATIM
)

add_custom_target(docs-all
  DEPENDS docs check-markdown-links
)
