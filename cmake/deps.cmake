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
  URL https://github.com/Agrael1/Wisdom/releases/download/Latest/Wisdom-0.6.8-win64.zip
  VERSION 0.6.8

  OPTIONS
  "WISDOM_BUILD_TESTS OFF"
  "WISDOM_BUILD_EXAMPLES OFF"
  "WISDOM_BUILD_DOCS OFF"
  "WISDOM_EXPERIMENTAL_CPP_MODULES ON"
)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${Wisdom_SOURCE_DIR}/lib/cmake/wisdom)
set(WISDOM_EXPERIMENTAL_CPP_MODULES ON CACHE BOOL "Enable experimental C++ modules support" FORCE)
find_package(Wisdom 0.6.7 REQUIRED)

# spdlog
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog 
  VERSION 1.15.1 
)

# SDL3
CPMAddPackage(
  NAME SDL3
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-3.2.10
  OPTIONS
  "SDL_WERROR OFF"
  "SDL_LEAN_AND_MEAN ON"
)


# zlib
CPMAddPackage(
  NAME ZLIB
  GITHUB_REPOSITORY madler/zlib
  GIT_TAG v1.3.1
  OPTIONS
  "ZLIB_COMPAT ON"
  "ZLIB_BUILD_TESTS OFF"
  "ZLIB_BUILD_EXAMPLES OFF"
)
# Crutch the ZLIB_INCLUDE_DIR to point to the source directory
set(ZLIB_INCLUDE_DIR ${ZLIB_SOURCE_DIR})
set(ZLIB_LIBRARY zlibstatic)
target_include_directories(zlibstatic PUBLIC ${ZLIB_BINARY_DIR})
add_library(ZLIB::ZLIB ALIAS zlibstatic)


# libpng
CPMAddPackage(
  NAME libpng
  GITHUB_REPOSITORY pnggroup/libpng
  GIT_TAG v1.6.40
  OPTIONS
  "PNG_TESTS OFF"
  "PNG_SHARED OFF"
  "SKIP_INSTALL_ALL ON"
)
target_include_directories(png_static INTERFACE ${libpng_SOURCE_DIR} ${libpng_BINARY_DIR})

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