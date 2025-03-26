set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${Wisdom_SOURCE_DIR}/lib/cmake/wisdom)
set(WISDOM_EXPERIMENTAL_CPP_MODULES ON CACHE BOOL "Enable experimental C++ modules support" FORCE)
find_package(Wisdom 0.6.7 REQUIRED)
#
#if(WIN32)
#  set(WISDOM_WINDOWS TRUE CACHE BOOL "Windows build" FORCE)
#  set(WISDOM_DX12 TRUE)
#elseif(APPLE)
#  set(WISDOM_MAC TRUE CACHE BOOL "Mac build" FORCE)
#elseif(UNIX AND NOT APPLE)
#  set(WISDOM_LINUX TRUE CACHE BOOL "Linux build" FORCE)
#endif()
#
#option(WISDOM_USE_WAYLAND "Use Wayland window system" ON)
#option(WISDOM_USE_X11 "Use X11 window system" ON)
#
#if(WISDOM_LINUX)
#  set(WISDOM_WAYLAND ${WISDOM_USE_WAYLAND})
#  set(WISDOM_X11 ${WISDOM_USE_X11})
#endif()
#
## Check for Vulkan
#find_package(Vulkan)
#if(Vulkan_FOUND)
#  set(WISDOM_VULKAN TRUE)
#endif()
#
#
#add_library(wisdom-modulex INTERFACE)
##target_sources(wisdom-modulex
##PUBLIC FILE_SET CXX_MODULES FILES 
##  #"${Wisdom_SOURCE_DIR}/include/wisdom/wisdom.ixx"
##  #"${Wisdom_SOURCE_DIR}/include/wisdom/generated/api/wisdom.api.ixx"
##)
#target_link_libraries(wisdom-modulex PUBLIC wis::wisdom-module)
#target_include_directories(wisdom-modulex PUBLIC ${Wisdom_SOURCE_DIR}/include)
#set_target_properties(wisdom-modulex PROPERTIES
#  CXX_STANDARD 23
#  CXX_STANDARD_REQUIRED ON
#  CXX_EXTENSIONS OFF
#  CXX_SCAN_FOR_MODULES ON
#)
#
#if (MSVC)
#  target_compile_options(wisdom-modulex PRIVATE "/Zc:__cplusplus")
#endif()
#
#if (WISDOM_DX12)
#	target_sources(wisdom-modulex
#	PUBLIC FILE_SET CXX_MODULES FILES 
#	  "${Wisdom_SOURCE_DIR}/include/wisdom/generated/dx12/wisdom.dx12.ixx"
#	)
#endif()
#
#if (WISDOM_VULKAN)
#	target_link_libraries(wisdom-modulex PUBLIC wis::wisdom-vk-headers)
#	target_sources(wisdom-modulex
#	PUBLIC FILE_SET CXX_MODULES FILES 
#	  "${Wisdom_SOURCE_DIR}/include/wisdom/generated/vulkan/wisdom.vk.ixx"
#	)
#endif()
#
#target_compile_definitions(wisdom-modulex 
#	PUBLIC 
#	$<$<BOOL:WISDOM_DX12>:WISDOM_DX12>
#	$<$<BOOL:WISDOM_VULKAN>:WISDOM_VULKAN>
#	$<$<BOOL:WISDOM_WAYLAND>:WIS_WAYLAND_PLATFORM>
#	$<$<BOOL:WISDOM_X11>:WIS_XCB_PLATFORM>
#	$<$<BOOL:WISDOM_X11>:WIS_X11_PLATFORM>
#	$<$<BOOL:WISDOM_WINDOWS>:WIS_WINDOWS_PLATFORM>
#	WISDOM_LOG_LEVEL=3
#)