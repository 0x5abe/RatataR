add_library(MinHook INTERFACE)

target_link_libraries(MinHook INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/libMinHook.x86.lib"
)

target_include_directories(MinHook INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)