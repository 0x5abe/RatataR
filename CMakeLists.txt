cmake_minimum_required(VERSION 3.16)
project(RatataR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RATATAR_OUTPUT_DIR "${CMAKE_BINARY_DIR}/RatataR")

add_subdirectory(external/minhook)
add_subdirectory(external/discord-rpc)
add_subdirectory(app)
add_subdirectory(hook)