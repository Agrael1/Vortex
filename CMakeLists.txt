cmake_minimum_required (VERSION 3.22)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141 AND MSVC)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("vortex")
include(cmake/deps.cmake)
include(cmake/shaders.cmake)

find_package(FFMPEG REQUIRED)

#if (NOT FFMPEG_FOUND)
#  message(FATAL_ERROR "FFMPEG not found. Please install FFMPEG or set FFMPEG_ROOT to the directory containing FFMPEG.")
#endif()

add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE "src")
target_sources(${PROJECT_NAME} 
PRIVATE
  "src/entry_main.cpp"
  
PUBLIC
  "src/vortex/app.h"
  "src/vortex/graphics.h"
  "src/vortex/util/common.h"
  "src/vortex/node.h"
  "src/vortex/node_registry.h"
  "src/vortex/ui/ui_app.h"
  "src/vortex/swapchain.h"

  "src/vortex/input/image_input.h"
  "src/vortex/util/log.h"
  
  "src/vortex/codec/codec_ffmpeg.h" 
  "src/vortex/codec/codec_ffmpeg.cpp" 
   
  "src/vortex/probe.h" 
  "src/vortex/pipeline_storage.h" 
  "src/vortex/gfx/texture.h"
  "src/vortex/gfx/texture.cpp"

  "src/vortex/ui/cef_app.h"
  "src/vortex/ui/sdl.h"
  "src/vortex/ui/implements.h"
  "src/vortex/ui/client.h" 
  "src/vortex/ui/ui_app.h"
 "src/vortex/ui/ui_app.cpp" "src/vortex/ui/sdl.cpp" "src/vortex/platform.cpp" "src/vortex/graphics.cpp" "src/vortex/gfx/descriptor_buffer.h" "src/vortex/gfx/consts.h" "src/vortex/gfx/descriptor_buffer.cpp" "src/vortex/util/reflection.h" "src/vortex/model.h" "src/vortex/ui/call_handler.h" "src/vortex/ui/cef_app.cpp" "src/vortex/ui/message_queue.h" )

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
    libcef_dll_wrapper
    
    FFmpeg::FFmpeg


    wis::wisdom-headers
    wis::wisdom-debug-headers
    wis::wisdom-extended-allocation-headers
    wis::wisdom-platform-headers
    wis::wisdom-descriptor-buffer-headers
)
target_precompile_headers(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/pch.hpp>)
add_dependencies(${PROJECT_NAME} shaders)

WIS_INSTALL_DEPS(${PROJECT_NAME})


# Copy the UI html files to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/rc/index.html"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/ui/index.html
)

# copy all dlls to the output directory
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
  )
  FFMPEG_COPY_DLL(${PROJECT_NAME})
endif()

# Copy the CEF resources to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CEF_SOURCE_DIR}/Resources"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if (WIN32)
  # Copy the CEF DLLs to the output directory
  # only copy dlls, bin and json files
  file(GLOB CEF_DLLS "${CEF_SOURCE_DIR}/Release/*.dll" "${CEF_SOURCE_DIR}/Release/*.bin" "${CEF_SOURCE_DIR}/Release/*.json")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CEF_DLLS}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
  set(MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rc")
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND "mt.exe" -nologo
            -manifest \"${MANIFEST_PATH}/${PROJECT_NAME}.exe.manifest\" \"${MANIFEST_PATH}/compatibility.manifest\"
            -outputresource:"$<TARGET_FILE:${PROJECT_NAME}>"\;\#1
    COMMENT "Adding manifest..."
  )
endif()
