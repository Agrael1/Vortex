# Wisdom
CPMAddPackage(
  NAME Wisdom
  GITHUB_REPOSITORY Agrael1/Wisdom
  GIT_TAG origin/master

  OPTIONS
  "WISDOM_BUILD_TESTS OFF"
  "WISDOM_BUILD_EXAMPLES OFF"
  "WISDOM_BUILD_DOCS OFF"
)

# FFMPEG (Fast Forward Moving Picture Experts Group)
CPMAddPackage(
  NAME FFMPEG
  URL https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-lgpl-shared.tar.xz
  VERSION latest
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${FFMPEG_SOURCE_DIR})
set(FFmpeg_INSTALL_PATH ${FFMPEG_SOURCE_DIR})

# SDL3 (Simple DirectMedia Layer)
CPMAddPackage(
  NAME SDL3
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.20
  OPTIONS
  "SDL_STATIC ON"
  "SDL_SHARED OFF"
  "SDL_TEST OFF"
  "SDL_EXAMPLES OFF"
)

# CEF (Chromium Embedded Framework)
CPMAddPackage(
  NAME CEF
  URL https://cef-builds.spotifycdn.com/cef_binary_139.0.40%2Bg465474a%2Bchromium-139.0.7258.139_linux64_minimal.tar.bz2
  VERSION 139.0.24
  OPTIONS
  "USE_SANDBOX OFF"
)
target_include_directories(libcef_dll_wrapper PUBLIC
  "${CEF_SOURCE_DIR}"
)
target_link_libraries(libcef_dll_wrapper PUBLIC 
  "${CEF_SOURCE_DIR}/Release/libcef.so"
)
