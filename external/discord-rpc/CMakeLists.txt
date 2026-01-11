add_library(discord-rpc INTERFACE)

target_link_libraries(MinHook INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/discord-rpc.lib"
)

target_include_directories(MinHook INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)