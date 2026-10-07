cmake_minimum_required(VERSION 3.20)
project(BankingQueueSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# Prefer local third_party header if present
find_path(NLOHMANN_JSON_INCLUDE_DIR "nlohmann/json.hpp"
  PATHS "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

if(NLOHMANN_JSON_INCLUDE_DIR)
  add_library(nlohmann_json_local INTERFACE)
  target_include_directories(nlohmann_json_local INTERFACE "${NLOHMANN_JSON_INCLUDE_DIR}")
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_local)
else()
  include(FetchContent)
  FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(json)
endif()

add_library(bank_core
  src/Desk.cpp
  src/QueueManager.cpp
  src/Bank.cpp
)
target_include_directories(bank_core PUBLIC include)

add_executable(bank_server src/server.cpp)
target_link_libraries(bank_server PRIVATE bank_core nlohmann_json::nlohmann_json Threads::Threads)
target_include_directories(bank_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

add_executable(bank_tests tests/queue_manager_tests.cpp)
target_link_libraries(bank_tests PRIVATE bank_core)

add_executable(service_flow_tests tests/service_flow_tests.cpp)
target_link_libraries(service_flow_tests PRIVATE bank_core)

enable_testing()
add_test(NAME bank_tests COMMAND bank_tests)
add_test(NAME service_flow_tests COMMAND service_flow_tests)
