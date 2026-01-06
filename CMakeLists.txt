cmake_minimum_required(VERSION 3.16)
project(RatataR VERSION 1.0.0)

add_library(hook SHARED hook.cpp)

target_link_libraries(hook
    Winmm
    d3d9
    ${CMAKE_SOURCE_DIR}/external/MinHook/lib/libMinHook.x86.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include/discord-rpc/lib/discord-rpc.lib
)

include_directories(${CMAKE_SOURCE_DIR}/external/MinHook/include)

add_custom_command(
    TARGET hook
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/rich-presence/include/discord-rpc/bin/discord-rpc.dll
            $<TARGET_FILE_DIR:hook>
)

target_include_directories(hook PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/discord-rpc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/rpc-client
)

set(DISCORD_RPC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/rich-presence/include/discord-rpc/lib/discord-rpc.lib")

# Print it to the console during configure
message(STATUS "Discord RPC library path: ${DISCORD_RPC_LIB}")

add_executable(RatataR WIN32
    RatataR.cpp
    RatataR.rc
)

target_link_libraries(RatataR
    Winmm
    kernel32
    user32
)

set_target_properties(RatataR PROPERTIES OUTPUT_NAME "RatataR")