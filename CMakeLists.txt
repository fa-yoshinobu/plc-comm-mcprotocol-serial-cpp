cmake_minimum_required(VERSION 3.20)

project(mcprotocol_serial_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MCPROTOCOL_BUILD_EXAMPLES "Build compile-checked example programs" ON)

add_library(mcprotocol_serial
  src/client.cpp
  src/codec.cpp
  src/posix_serial.cpp
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
endif()

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
  set(MCPROTOCOL_DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  set(MCPROTOCOL_DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  configure_file(${MCPROTOCOL_DOXYFILE_IN} ${MCPROTOCOL_DOXYFILE_OUT} @ONLY)

  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${MCPROTOCOL_DOXYFILE_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
else()
  add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not found. Install doxygen and rebuild the docs target."
    COMMENT "Doxygen is not installed; docs target is informational only"
    VERBATIM
  )
endif()
