cmake_minimum_required(VERSION 3.16)
project(RatataR VERSION 1.0.0)

add_library(hook SHARED hook.cpp)
target_link_libraries(hook Winmm "${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include/discord-rpc/lib/discord-rpc.lib")

add_executable(RatataR WIN32
    RatataR.cpp
    RatataR.rc
)

target_include_directories(RatataR PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/discord-rpc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client
)

set(DISCORD_RPC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include/discord-rpc/lib/discord-rpc.lib")

# Print it to the console during configure
message(STATUS "Discord RPC library path: ${DISCORD_RPC_LIB}")

target_link_libraries(RatataR
    Winmm
    kernel32
    user32
    "${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include/discord-rpc/lib/discord-rpc.lib"
)

set_target_properties(RatataR PROPERTIES OUTPUT_NAME "RatataR")
