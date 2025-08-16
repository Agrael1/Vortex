include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(CPM_DONT_UPDATE_MODULE_PATH ON)
set(GET_CPM_FILE "${CMAKE_CURRENT_LIST_DIR}/get_cpm.cmake")

if (NOT EXISTS ${GET_CPM_FILE})
  file(DOWNLOAD
      https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
      "${GET_CPM_FILE}"
  )
endif()
include(${GET_CPM_FILE})

# Add CPM dependencies here
# Wisdom
CPMAddPackage(
  NAME Wisdom
  URL https://github.com/Agrael1/Wisdom/releases/download/0.6.11/Wisdom-0.6.11-win64.zip
  VERSION 0.6.11

  OPTIONS
  "WISDOM_BUILD_TESTS OFF"
  "WISDOM_BUILD_EXAMPLES OFF"
  "WISDOM_BUILD_DOCS OFF"
  "WISDOM_EXPERIMENTAL_CPP_MODULES ON"
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${Wisdom_SOURCE_DIR}/lib/cmake/wisdom)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(WISDOM_EXPERIMENTAL_CPP_MODULES ON CACHE BOOL "Enable experimental C++ modules support" FORCE)
find_package(Wisdom 0.6.10 REQUIRED)

# spdlog
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog 
  VERSION 1.15.1 
)

# FFMPEG
CPMAddPackage(
  NAME FFMPEG
  URL https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-lgpl-shared.zip
  VERSION latest
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${FFMPEG_SOURCE_DIR})
set(FFmpeg_INSTALL_PATH ${FFMPEG_SOURCE_DIR})


# SDL3 imported target
CPMAddPackage(
  NAME SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.20/SDL3-devel-3.2.20-VC.zip
  VERSION release-3.2.20
  OPTIONS
  DOWNLOAD_ONLY ON
)

# SDL3
add_library(SDL3::SDL3 SHARED IMPORTED)
set_target_properties(SDL3::SDL3 PROPERTIES
  IMPORTED_LOCATION "${SDL3_SOURCE_DIR}/lib/x64/SDL3.dll"
  IMPORTED_IMPLIB "${SDL3_SOURCE_DIR}/lib/x64/SDL3.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${SDL3_SOURCE_DIR}/include"
)


# Cef
CPMAddPackage(
  NAME CEF
  URL https://cef-builds.spotifycdn.com/cef_binary_138.0.15%2Bgd0f1f64%2Bchromium-138.0.7204.50_windows64_minimal.tar.bz2
  VERSION 138.0.15+gd0f1f64+chromium-138.0.7204.50
  OPTIONS
  "USE_SANDBOX OFF"
  "CEF_RUNTIME_LIBRARY_FLAG /MD"
)
target_include_directories(libcef_dll_wrapper PUBLIC
  "${CEF_SOURCE_DIR}"
)
target_link_libraries(libcef_dll_wrapper PUBLIC 
  "${CEF_SOURCE_DIR}/Release/libcef.lib"
)

# frozen
CPMAddPackage(
  NAME frozen
  GITHUB_REPOSITORY serge-sans-paille/frozen
  GIT_TAG 1.2.0
)

# NDI SDK
if (NOT DEFINED ENV{NDI_SDK_DIR})
  set(NDI_SDK_DIR)
  message(WARNING "NDI_SDK_DIR environment variable not set. NDI SDK will not be available.")

  # Add dummy imported target for NDI SDK
  add_library(NDI INTERFACE)
  add_library(NDI::NDI ALIAS NDI)
else()
  set(NDI_SDK_DIR $ENV{NDI_SDK_DIR})
  message(STATUS "NDI_SDK_DIR: ${NDI_SDK_DIR}")

  # Create imported target for NDI SDK
  add_library(NDI SHARED IMPORTED)
  add_library(NDI::NDI ALIAS NDI)

  target_include_directories(NDI INTERFACE
    "${NDI_SDK_DIR}/Include"
  )
  target_compile_definitions(NDI INTERFACE
    "NDI_AVAILABLE"
  )

  # Set the location of the NDI SDK library (for now win only)
  if (WIN32)
    set_target_properties(NDI PROPERTIES
      IMPORTED_LOCATION "${NDI_SDK_DIR}/Bin/x64/Processing.NDI.Lib.x64.dll"
      IMPORTED_IMPLIB "${NDI_SDK_DIR}/Lib/x64/Processing.NDI.Lib.x64.lib"
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    )
  endif()
endif()

if (VORTEX_GENERATE_PROPERTIES)
  # tinyxml2
  CPMAddPackage(
    NAME tinyxml2
    GITHUB_REPOSITORY leethomason/tinyxml2
    GIT_TAG master
    OPTIONS
    "tinyxml2_BUILD_TESTING OFF"
  )
endif()

# catch2
if(VORTEX_BUILD_TESTS)
  CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    GIT_TAG v3.4.0
    OPTIONS
    "CATCH_BUILD_TESTING OFF"
    "CATCH_INSTALL_DOCS OFF"
  )
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()