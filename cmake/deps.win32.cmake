# Wisdom
CPMAddPackage(
  NAME Wisdom
  URL https://github.com/Agrael1/Wisdom/releases/download/0.6.12/Wisdom-0.6.12-win64.zip
  VERSION 0.6.12

  OPTIONS
  "WISDOM_BUILD_TESTS OFF"
  "WISDOM_BUILD_EXAMPLES OFF"
  "WISDOM_BUILD_DOCS OFF"
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${Wisdom_SOURCE_DIR}/lib/cmake/wisdom)
find_package(Wisdom 0.6.12 REQUIRED)

#FFMPEG
CPMAddPackage(
  NAME FFMPEG
  URL https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-lgpl-shared.zip
  VERSION latest
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${FFMPEG_SOURCE_DIR})
set(FFmpeg_INSTALL_PATH ${FFMPEG_SOURCE_DIR})



# SDL3
CPMAddPackage(
  NAME SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.20/SDL3-devel-3.2.20-VC.zip
  VERSION release-3.2.20
  OPTIONS
  DOWNLOAD_ONLY ON
)
add_library(SDL3::SDL3 SHARED IMPORTED)
set_target_properties(SDL3::SDL3 PROPERTIES
  IMPORTED_LOCATION "${SDL3_SOURCE_DIR}/lib/x64/SDL3.dll"
  IMPORTED_IMPLIB "${SDL3_SOURCE_DIR}/lib/x64/SDL3.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${SDL3_SOURCE_DIR}/include"
)

# CEF (Chromium Embedded Framework)
CPMAddPackage(
  NAME CEF
  URL https://cef-builds.spotifycdn.com/cef_binary_139.0.24%2Bgce684ab%2Bchromium-139.0.7258.128_windows64_minimal.tar.bz2
  VERSION 139.0.24
  OPTIONS
  "USE_SANDBOX OFF"
  "CEF_RUNTIME_LIBRARY_FLAG /MD"
)
#CPMAddPackage(
#  NAME CEFPDB
#  URL https://cef-builds.spotifycdn.com/cef_binary_139.0.24%2Bgce684ab%2Bchromium-139.0.7258.128_windows64_release_symbols.tar.bz2
#  VERSION 139.0.24
#  DOWNLOAD_ONLY ON
#)
target_include_directories(libcef_dll_wrapper PUBLIC
  "${CEF_SOURCE_DIR}"
)
target_link_libraries(libcef_dll_wrapper PUBLIC 
  "${CEF_SOURCE_DIR}/Release/libcef.lib"
)