cmake_minimum_required (VERSION 3.22)

option(VORTEX_GENERATE_PROPERTIES "Build generator and generate properties" ON)
option(VORTEX_BUILD_TESTS "Build tests" ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141 AND MSVC)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("vortex")
include(cmake/deps.cmake)
include(cmake/shaders.cmake)

find_package(FFMPEG REQUIRED)

if (UNIX AND NOT APPLE)
  set(VORTEX_LINUX TRUE)
  find_package(X11 REQUIRED)
endif()

if (VORTEX_GENERATE_PROPERTIES)
  add_subdirectory(gen)
endif()


add_library(VortexLib STATIC)
target_sources(VortexLib
PUBLIC
# "src/vortex/pch.hpp"

  "src/vortex/gfx/swapchain.h"
  "src/vortex/gfx/descriptor_buffer.h" 
  "src/vortex/gfx/descriptor_buffer.cpp" 
  "src/vortex/gfx/texture.cpp"
  "src/vortex/gfx/texture.h"
  "src/vortex/gfx/texture_pool.cpp"
  "src/vortex/gfx/texture_pool.h"
 
  "src/vortex/graph/optimize_probe.h" 
  "src/vortex/graph/optimize_probe.cpp"
  "src/vortex/graph/ports.h" 
  "src/vortex/graph/connection.h" 
  "src/vortex/graph/interfaces.h" 
  "src/vortex/graph/node_factory.h" 
 
  "src/vortex/util/lib/SPSC-Queue.h" 
  "src/vortex/util/reflection.h" 
  "src/vortex/util/log.h"
  "src/vortex/util/ndi/ndi_library.h"  
  "src/vortex/util/ndi/ndi_swapchain.h" 
  "src/vortex/util/ndi/ndi_swapchain.cpp"
  "src/vortex/util/rational.h"
  "src/vortex/util/unique_any.h"
  "src/vortex/util/common.h"
 
  "src/vortex/nodes/input/image_input.h"
  "src/vortex/nodes/input/image_input.cpp"
  "src/vortex/nodes/input/stream_input.h"
  "src/vortex/nodes/input/stream_input.cpp"
  "src/vortex/nodes/filter/composition.h"
  "src/vortex/nodes/filter/blend.h"
  "src/vortex/nodes/node_registry.h"
 
  "src/vortex/codec/ffmpeg/types.h" 
  "src/vortex/codec/ffmpeg/error.h" 
  "src/vortex/codec/ffmpeg/stream_manager.h" 
  "src/vortex/codec/ffmpeg/stream_manager.cpp" 
  "src/vortex/codec/ffmpeg/hw_decoder.h" 
  "src/vortex/codec/ffmpeg/audio_resampler.h" 
  "src/vortex/codec/ffmpeg/audio_resampler.cpp"
  "src/vortex/codec/ffmpeg/codec_ffmpeg.h" 
  "src/vortex/codec/ffmpeg/codec_ffmpeg.cpp" 
 
  "src/vortex/sync/wall_clock.h"
  "src/vortex/sync/timeline.h"
 
  "src/vortex/ui/sdl.h"
  "src/vortex/ui/sdl.cpp"
  
  "src/vortex/consts.h" 
  "src/vortex/probe.h" 
  
  "src/vortex/platform.cpp" 
  "src/vortex/platform.h" 
  "src/vortex/graphics.cpp" 
  "src/vortex/graphics.h"
  "src/vortex/model.h" 
  "src/vortex/model.cpp" 
  
  "src/vortex/util/lazy.h"  
  "src/vortex/util/interprocess_lock.h" 
  "src/vortex/util/interprocess_lock.cpp" 
  "src/vortex/nodes/output/ndi_output.cpp" 
  "src/vortex/audio/audio_buffer.h" 
  "src/vortex/util/byte_ring.h"  
  "src/vortex/audio/audio_resampler.h" 
  "src/vortex/ui/message_dispatch.h" 
  "src/vortex/util/bench_clock.h" 

 "src/vortex/util/main_args.h")

target_include_directories(VortexLib PUBLIC "src")
set_target_properties(VortexLib PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
# Add the /Zc:__cplusplus flag
target_compile_options(VortexLib PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)
target_link_libraries(VortexLib
  PUBLIC
    spdlog::spdlog_header_only
    SDL3::SDL3
    NDI::NDI
    
    FFmpeg::FFmpeg
    frozen::frozen-headers
    DirectXMath

    wis::wisdom-headers
    wis::wisdom-debug-headers
    wis::wisdom-extended-allocation-headers
    wis::wisdom-platform-headers
    wis::wisdom-descriptor-buffer-headers

    #x11 for linux
    $<$<BOOL:${VORTEX_LINUX}>:${X11_LIBRARIES}>
)
#target_compile_definitions(VortexLib PUBLIC WISDOM_FORCE_VULKAN=1)
target_precompile_headers(VortexLib PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/pch.hpp>)


add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_sources(${PROJECT_NAME} 
PRIVATE
  "src/entry_main.cpp"

  "src/vortex/ui/client.h" 
  "src/vortex/ui/implements.h"
  "src/vortex/ui/cef/app.h"
  "src/vortex/ui/cef/app.cpp"
  "src/vortex/ui/ui_app.h"
  "src/vortex/ui/ui_app.cpp"
  "src/vortex/ui/value.h" 
  "src/vortex/ui/call_handler.h"  

  "src/vortex/app.h"
)
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wno-interference-size>
  $<$<CXX_COMPILER_ID:Clang>:-Wno-interference-size>
)

add_custom_target(ui)
# Copy the UI html files to the output directory
add_custom_command(TARGET ui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/rc"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/ui
)

#if (VORTEX_GENERATE_PROPERTIES)
#  add_custom_target(properties)
#  set(PROP_FILES 
#    "${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/xml/props.xml"
#  )
#  target_sources(properties
#    PRIVATE ${PROP_FILES}
#  )
#
#  # Generate the properties file
#  foreach (PROP_FILE ${PROP_FILES})
#    get_filename_component(PROP_NAME ${PROP_FILE} NAME_WE)
#    add_custom_command(TARGET properties PRE_BUILD
#      COMMAND $<TARGET_FILE:generator> ${PROP_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/properties/${PROP_NAME}.hpp
#      COMMENT "Generating properties for ${PROP_NAME}"
#    )
#  endforeach()
#  add_dependencies(${PROJECT_NAME} properties)
#endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  # Set binary output dir to bin
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# Add the /Zc:__cplusplus flag
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)
target_link_libraries(${PROJECT_NAME} 
  PUBLIC 
    VortexLib
    libcef_dll_wrapper
)
#target_compile_definitions(${PROJECT_NAME} PUBLIC WISDOM_FORCE_VULKAN=1)
#target_precompile_headers(${PROJECT_NAME} REUSE_FROM VortexLib)
add_dependencies(${PROJECT_NAME} shaders ui)

WIS_INSTALL_DEPS(${PROJECT_NAME})



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

# Copy the CEF DLLs to the output directory
# only copy dlls, bin and json files
file(GLOB CEF_DLLS "${CEF_SOURCE_DIR}/Release/*.dll" "${CEF_SOURCE_DIR}/Release/*.bin" "${CEF_SOURCE_DIR}/Release/*.json")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CEF_DLLS}
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if (WIN32)
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

if (VORTEX_BUILD_TESTS)
enable_testing()
add_subdirectory(test)
endif()