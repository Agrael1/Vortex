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
  URL https://github.com/Agrael1/Wisdom/releases/download/Latest/Wisdom-0.6.7-win64.zip
  VERSION 0.6.7

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