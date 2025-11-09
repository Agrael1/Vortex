cmake_minimum_required (VERSION 3.22)

option(VORTEX_GENERATE_PROPERTIES "Build generator and generate properties" ON)
option(VORTEX_BUILD_TESTS "Build tests" ON)
option(VORTEX_WITH_NDI "Build with NDI support" OFF)

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
PRIVATE
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
  "src/vortex/util/ndi/ndi_library.h"
  "src/vortex/util/ndi/ndi_swapchain.h"
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

target_include_directories(VortexLib PUBLIC "src")
set_target_properties(VortexLib PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_compile_options(VortexLib PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)

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

# ---- NDI toggle ----
if(VORTEX_WITH_NDI)
  message(STATUS "VORTEX_WITH_NDI=ON — enabling NDI sources and linkage")
  target_sources(VortexLib PRIVATE
    "src/vortex/util/ndi/ndi_swapchain.cpp"
    "src/vortex/nodes/output/ndi_output.cpp"
  )
  target_compile_definitions(VortexLib PUBLIC VORTEX_HAS_NDI=1)

  if(TARGET NDI::NDI)
    target_link_libraries(VortexLib PUBLIC NDI::NDI)
  else()
    if(NOT DEFINED ENV{NDI_SDK_DIR})
      if(EXISTS "C:/Program Files/NDI/NDI 6 SDK/Include")
        set(ENV{NDI_SDK_DIR} "C:/Program Files/NDI/NDI 6 SDK")
      elseif(EXISTS "C:/Program Files/NDI/NDI SDK for Windows/Include")
        set(ENV{NDI_SDK_DIR} "C:/Program Files/NDI/NDI SDK for Windows")
      endif()
    endif()
    if(DEFINED ENV{NDI_SDK_DIR})
      message(STATUS "Using NDI SDK at: $ENV{NDI_SDK_DIR}")
      target_include_directories(VortexLib PRIVATE "$ENV{NDI_SDK_DIR}/Include")
      if(EXISTS "$ENV{NDI_SDK_DIR}/Lib/Windows/x64/Processing.NDI.Lib.x64.lib")
        target_link_directories(VortexLib PRIVATE "$ENV{NDI_SDK_DIR}/Lib/Windows/x64")
      elseif(EXISTS "$ENV{NDI_SDK_DIR}/Lib/x64/Processing.NDI.Lib.x64.lib")
        target_link_directories(VortexLib PRIVATE "$ENV{NDI_SDK_DIR}/Lib/x64")
      endif()
      target_link_libraries(VortexLib PUBLIC Processing.NDI.Lib.x64)
    else()
      message(FATAL_ERROR "VORTEX_WITH_NDI=ON, но NDI::NDI не найден и NDI_SDK_DIR не задан.")
    endif()
  endif()
else()
  message(STATUS "VORTEX_WITH_NDI=OFF — building WITHOUT NDI")
  target_compile_definitions(VortexLib PUBLIC VORTEX_HAS_NDI=0)
endif()
# ---- /NDI toggle ----

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

# --- UI: копируем собранный фронтенд в runtime/ui ---
# Поддерживаем и rc/ui, и rc/
set(VORTEX_UI_SRC "")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rc/ui/index.html")
  set(VORTEX_UI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/rc/ui")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rc/index.html")
  set(VORTEX_UI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/rc")
endif()

add_custom_target(vortex_ui_copy ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
  COMMENT "Preparing UI folder..."
)

if(VORTEX_UI_SRC)
  add_custom_command(TARGET vortex_ui_copy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VORTEX_UI_SRC}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
    COMMENT "Copying UI from ${VORTEX_UI_SRC} -> $<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
  )
else()
  add_custom_command(TARGET vortex_ui_copy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[vortex] UI not found at rc/ or rc/ui. Build it with: cd vortex-ui-skeleton && npm run build -- --mode rc"
  )
endif()

# зависимости
add_dependencies(${PROJECT_NAME} shaders vortex_ui_copy)

# Встраиваем манифесты ОДИН РАЗ и ТОЛЬКО если файлы есть
if (WIN32)
  set(MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rc")
  set(APP_MAN "${MANIFEST_PATH}/${PROJECT_NAME}.exe.manifest")
  set(COMP_MAN "${MANIFEST_PATH}/compatibility.manifest")
  if (EXISTS "${APP_MAN}" AND EXISTS "${COMP_MAN}")
    # CMake может знать путь к mt.exe как CMAKE_MT_EXECUTABLE (в VS toolset).
    if (NOT CMAKE_MT_EXECUTABLE)
      set(CMAKE_MT_EXECUTABLE mt.exe)
    endif()
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND "${CMAKE_MT_EXECUTABLE}" -nologo
              -manifest "\"${APP_MAN}\"" "\"${COMP_MAN}\""
              -outputresource:"$<TARGET_FILE:${PROJECT_NAME}>";#1
      COMMENT "Embedding manifests..."
    )
  else()
    message(WARNING "Manifest files not found in ${MANIFEST_PATH}; skipping mt.exe embed.")
  endif()
endif()

if (VORTEX_GENERATE_PROPERTIES)
  add_custom_target(properties)
  set(PROP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/xml/props.xml")
  target_sources(properties PRIVATE ${PROP_FILES})
  foreach (PROP_FILE ${PROP_FILES})
    get_filename_component(PROP_NAME ${PROP_FILE} NAME_WE)
    add_custom_command(TARGET properties PRE_BUILD
      COMMAND $<TARGET_FILE:prop_generator> ${PROP_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/src/vortex/properties/${PROP_NAME}.hpp
      COMMENT "Generating properties for ${PROP_NAME}"
    )
  endforeach()
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /wd5050>)

# ---- WHOLEARCHIVE для регистрации нод ----
if (MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE "/WHOLEARCHIVE:VortexLib")
  target_link_libraries(${PROJECT_NAME} PUBLIC libcef_dll_wrapper VortexLib)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC -Wl,--whole-archive VortexLib -Wl,--no-whole-archive libcef_dll_wrapper
  )
endif()

add_dependencies(${PROJECT_NAME} shaders vortex_ui_copy)
WIS_INSTALL_DEPS(${PROJECT_NAME})

if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
  )
  FFMPEG_COPY_DLL(${PROJECT_NAME})
endif()

# Копирование ресурсов CEF
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CEF_SOURCE_DIR}/Resources"
          $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
file(GLOB CEF_DLLS "${CEF_SOURCE_DIR}/Release/*.dll" "${CEF_SOURCE_DIR}/Release/*.bin" "${CEF_SOURCE_DIR}/Release/*.json")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CEF_DLLS}
          $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Копирование NDI DLL (если включен NDI)
if (WIN32 AND VORTEX_WITH_NDI)
  set(_ndi_dll "")
  if(DEFINED ENV{NDI_SDK_DIR})
    if(EXISTS "$ENV{NDI_SDK_DIR}/Bin/x64/Processing.NDI.Lib.x64.dll")
      set(_ndi_dll "$ENV{NDI_SDK_DIR}/Bin/x64/Processing.NDI.Lib.x64.dll")
    elseif(EXISTS "$ENV{NDI_SDK_DIR}/Redist/Windows/x64/Processing.NDI.Lib.x64.dll")
      set(_ndi_dll "$ENV{NDI_SDK_DIR}/Redist/Windows/x64/Processing.NDI.Lib.x64.dll")
    endif()
  endif()
  if(_ndi_dll)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_ndi_dll}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMENT "Copying NDI runtime DLL"
    )
  endif()
endif()

if (VORTEX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
