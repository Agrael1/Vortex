cmake_minimum_required (VERSION 3.22)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141 AND MSVC)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Vortex")
include(cmake/deps.cmake)

add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE "include")
target_sources(${PROJECT_NAME} 
PRIVATE
  "src/entry_main.cpp"
  
PUBLIC
  "include/vortex/app.h"
  "include/vortex/graphics.h"
  "include/util/common.h"
  "include/vortex/node.h"
  "include/vortex/node_registry.h"
  "include/vortex/platform.h"
  "include/vortex/swapchain.h"

  "include/vortex/codec/codec_png.h"
  "include/vortex/codec/codec_png.cpp"

  "include/vortex/input/image_input.h"
  "include/util/log.h"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  # Set binary output dir to bin
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# Add the /Zc:__cplusplus flag
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    spdlog::spdlog
    SDL3::SDL3
    NDI::NDI
    png_static
    
    wis::wisdom-headers
    wis::wisdom-debug-headers
    wis::wisdom-extended-allocation-headers
    wis::wisdom-platform-headers
    wis::wisdom-descriptor-buffer-headers
)
target_precompile_headers(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include/vortex/pch.hpp>)

WIS_INSTALL_DEPS(${PROJECT_NAME})

# copy all dlls to the output directory
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
  )
endif()