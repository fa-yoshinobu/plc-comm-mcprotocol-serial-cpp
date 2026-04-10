cmake_minimum_required(VERSION 3.20)

project(mcprotocol_serial_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
