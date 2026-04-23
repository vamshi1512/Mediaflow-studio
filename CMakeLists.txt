cmake_minimum_required(VERSION 3.25)

project(MediaFlowStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)

option(MEDIAFLOW_BUILD_TESTS "Build MediaFlow Studio tests" ON)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_Declare(
  cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.5
)

if(MEDIAFLOW_BUILD_TESTS)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  )
endif()

FetchContent_MakeAvailable(nlohmann_json cpphttplib)

if(MEDIAFLOW_BUILD_TESTS)
  FetchContent_MakeAvailable(googletest)
endif()

add_library(
  mediaflow_core
  src/config.cpp
  src/frame_generator.cpp
  src/http_server.cpp
  src/jitter_buffer.cpp
  src/logger.cpp
  src/metrics.cpp
  src/network_simulator.cpp
  src/receiver.cpp
  src/scenario_factory.cpp
  src/sender.cpp
  src/simulation_engine.cpp
)

target_include_directories(
  mediaflow_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(TARGET httplib)
  target_link_libraries(mediaflow_core PUBLIC httplib)
else()
  target_include_directories(mediaflow_core PUBLIC ${cpphttplib_SOURCE_DIR})
endif()

target_link_libraries(mediaflow_core PUBLIC nlohmann_json::nlohmann_json)

if(MSVC)
  target_compile_options(mediaflow_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    mediaflow_core
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wconversion
      -Wsign-conversion
      -Wnon-virtual-dtor
  )
endif()

add_executable(mediaflow src/main.cpp)
target_link_libraries(mediaflow PRIVATE mediaflow_core)

if(MEDIAFLOW_BUILD_TESTS)
  enable_testing()
  include(GoogleTest)

  add_executable(
    mediaflow_tests
    tests/config_loader_tests.cpp
    tests/deterministic_replay_tests.cpp
    tests/integration_scenario_tests.cpp
    tests/jitter_buffer_tests.cpp
    tests/metrics_tests.cpp
    tests/receiver_tests.cpp
    tests/sender_tests.cpp
  )

  target_link_libraries(mediaflow_tests PRIVATE mediaflow_core GTest::gtest_main)
  target_compile_definitions(
    mediaflow_tests
    PRIVATE MEDIAFLOW_TEST_CONFIG_DIR="${PROJECT_SOURCE_DIR}/configs"
  )
  gtest_discover_tests(mediaflow_tests)
endif()
