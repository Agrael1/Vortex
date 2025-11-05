cmake_minimum_required (VERSION 3.22)

option(VORTEX_GENERATE_PROPERTIES "Build generator and generate properties" ON)
option(VORTEX_BUILD_TESTS "Build tests" ON)
option(VORTEX_BUILD_SHADERS "Compile HLSL shaders via Wisdom" ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141 AND MSVC)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,\
      $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,\
      $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("vortex")

# ---------------------------
# CEF source dir detection
# ---------------------------
set(_cef_candidates
  "${CEF_ROOT}"
  "${CEF_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/_deps/cef-src"
  "${CMAKE_BINARY_DIR}/_deps/cef"
  "${CMAKE_SOURCE_DIR}/third_party/cef"
)
unset(CEF_SOURCE_DIR CACHE)
foreach(_p IN LISTS _cef_candidates)
  if(_p AND EXISTS "${_p}/Resources/icudtl.dat")
    set(CEF_SOURCE_DIR "${_p}")
    break()
  endif()
endforeach()

if(NOT CEF_SOURCE_DIR)
  message(FATAL_ERROR
    "Не нашёл CEF_SOURCE_DIR. Проверь, что deps.cmake подтягивает CEF через CPM, "
    "или укажи CEF_ROOT в пресете на распакованный дистрибутив.")
endif()
message(STATUS "CEF SOURCE DIR: ${CEF_SOURCE_DIR}")

# ---------------------------
# Dependencies (CPM, etc)
# ---------------------------
include(cmake/deps.cmake)

# ---------------------------
# HLSL shaders via Wisdom (опционально)
# ---------------------------
if (VORTEX_BUILD_SHADERS)
  include(cmake/shaders.cmake)
endif()

# ---------------------------
# FFmpeg (required)
# ---------------------------
find_package(FFMPEG REQUIRED)

# ---------------------------
# Linux specifics
# ---------------------------
if (UNIX AND NOT APPLE)
  set(VORTEX_LINUX TRUE)
  find_package(X11 REQUIRED)
endif()

# ---------------------------
# Генератор пропертей (опционально)
# ---------------------------
if (VORTEX_GENERATE_PROPERTIES)
  add_subdirectory(gen)
endif()

# ---------------------------
# NDI (опционально)
#   - deps.cmake создаёт таргет NDI::NDI только при наличии SDK.
#   - Включаем NDI ТОЛЬКО если есть и таргет, и переменная окружения NDI_SDK_DIR.
# ---------------------------
set(NDI_AVAILABLE FALSE)
if (TARGET NDI::NDI AND DEFINED ENV{NDI_SDK_DIR} AND EXISTS "$ENV{NDI_SDK_DIR}")
  set(NDI_AVAILABLE TRUE)
else()
  message(WARNING "NDI не найден (нет NDI::NDI или NDI_SDK_DIR). Собираем без поддержки NDI.")
endif()

# ---------------------------
# Источники библиотеки
# ---------------------------
set(VORTEXLIB_SOURCES
  # "src/vortex/pch.hpp"

  "src/vortex/gfx/swapchain.h"
  "src/vortex/gfx/descriptor_buffer.h"
  "src/vortex/gfx/descriptor_buffer.cpp"
  "src/vortex/gfx/texture.cpp"
  "src/vortex/gfx/texture.h"
  "src/vortex/gfx/texture_pool.cpp"
  "src/vortex/gfx/texture_pool.h"

  "src/vortex/graph/ports.h"
  "src/vortex/graph/connection.h"
  "src/vortex/graph/interfaces.h"
  "src/vortex/graph/node_factory.h"
  "src/vortex/graph/output_scheduler.h"
  "src/vortex/graph/output_scheduler.cpp"

  "src/vortex/util/lib/SPSC-Queue.h"
  "src/vortex/util/reflection.h"
  "src/vortex/util/log.h"
  "src/vortex/util/rational.h"
  "src/vortex/util/unique_any.h"
  "src/vortex/util/common.h"

  "src/vortex/nodes/input/image_input.h"
  "src/vortex/nodes/input/image_input.cpp"
  "src/vortex/nodes/input/stream_input.h"
  "src/vortex/nodes/input/stream_input.cpp"

  "src/vortex/nodes/filter/blend.h"
  "src/vortex/nodes/filter/blend.cpp"
  "src/vortex/nodes/filter/select.h"
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
  "src/vortex/audio/audio_buffer.h"
  "src/vortex/audio/audio_resampler.h"
  "src/vortex/util/byte_ring.h"
  "src/vortex/anim/animation.h"
  "src/vortex/anim/animation.cpp"
  "src/vortex/ui/message_dispatch.h"
  "src/vortex/util/bench_clock.h"

  "src/vortex/util/term/log_sink.h"
  "src/vortex/util/term/log_sink.cpp"
  "src/vortex/util/term/input.h"
  "src/vortex/util/term/impl.win32.h"
  "src/vortex/util/term/impl.win32.cpp"
  "src/vortex/util/term/impl.posix.cpp"
  "src/vortex/util/term/shared.h"
  "src/vortex/util/term/input.cpp"

  "src/vortex/util/main_args.h"
  "src/vortex/nodes/output/window_output.cpp"
  "src/vortex/util/interp/interpolation.h"
  "src/vortex/util/interp/easing.h"
  "src/vortex/util/interp/vector.h"
  "src/vortex/anim/keyframe.h"
  "src/vortex/anim/keyframe.cpp"
  "src/vortex/anim/property_track.h"
  "src/vortex/anim/property_track.cpp"
  "src/vortex/anim/animation_clip.h"
  "src/vortex/anim/animation_clip.cpp"
  "src/vortex/util/deserialize.h"
)

# NDI-файлы добавляем только если SDK доступен
if (NDI_AVAILABLE)
  list(APPEND VORTEXLIB_SOURCES
    "src/vortex/util/ndi/ndi_library.h"
    "src/vortex/util/ndi/ndi_swapchain.h"
    "src/vortex/util/ndi/ndi_swapchain.cpp"
    "src/vortex/nodes/output/ndi_output.cpp"
  )
endif()

# ---------------------------
# VortexLib
# ---------------------------
add_library(VortexLib STATIC)
target_sources(VortexLib PUBLIC ${VORTEXLIB_SOURCES})
target_include_directories(VortexLib PUBLIC "src")
set_target_properties(VortexLib PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
# Add the /Zc:__cplusplus flag
target_compile_options(VortexLib PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)

# Дефайны для наличия/отсутствия NDI
if (NDI_AVAILABLE)
  target_compile_definitions(VortexLib PUBLIC VORTEX_HAS_NDI=1)
else()
  target_compile_definitions(VortexLib PUBLIC VORTEX_HAS_NDI=0)
endif()

target_link_libraries(VortexLib
  PUBLIC
    spdlog::spdlog_header_only
    SDL3::SDL3
    utf8cpp
    nlohmann_json::nlohmann_json
    FFmpeg::FFmpeg
    frozen::frozen-headers
    DirectXMath
    wis::wisdom-headers
    wis::wisdom-debug-headers
    wis::wisdom-extended-allocation-headers
    wis::wisdom-platform-headers
    wis::wisdom-descriptor-buffer-headers
    $<$<BOOL:${VORTEX_LINUX}>:${X11_LIBRARIES}>
)

# Линкуем NDI только если доступен
if (NDI_AVAILABLE)
  target_link_libraries(VortexLib PUBLIC NDI::NDI)
endif()

target_precompile_headers(VortexLib PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/pch.hpp>)

# ---------------------------
# Executable
# ---------------------------
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

# Дефайн для exe — в паре с либой
if (NDI_AVAILABLE)
  target_compile_definitions(${PROJECT_NAME} PUBLIC VORTEX_HAS_NDI=1)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC VORTEX_HAS_NDI=0)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wno-interference-size>
  $<$<CXX_COMPILER_ID:Clang>:-Wno-interference-size>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    VortexLib
    libcef_dll_wrapper
)

# ---------------------------
# UI assets copy
# ---------------------------
add_custom_target(ui)
add_custom_command(TARGET ui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/rc"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/ui
)

# ---------------------------
# Шейдеры и UI должны собираться до exe
# ---------------------------
if (VORTEX_BUILD_SHADERS)
  add_dependencies(${PROJECT_NAME} shaders)
endif()
add_dependencies(${PROJECT_NAME} ui)

# ---------------------------
# Wisdom helper: copy deps
# ---------------------------
WIS_INSTALL_DEPS(${PROJECT_NAME})

# ---------------------------
# Copy runtime DLLs (Windows)
# ---------------------------
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
  )
  FFMPEG_COPY_DLL(${PROJECT_NAME})
endif()

# ---------------------------
# Copy CEF resources and DLLs
# ---------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CEF_SOURCE_DIR}/Resources"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

file(GLOB CEF_DLLS
  "${CEF_SOURCE_DIR}/Release/*.dll"
  "${CEF_SOURCE_DIR}/Release/*.bin"
  "${CEF_SOURCE_DIR}/Release/*.json"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CEF_DLLS}
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# ---------------------------
# Embed manifest (Windows)
# ---------------------------
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

# ---------------------------
# Tests
# ---------------------------
if (VORTEX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
