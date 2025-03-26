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
include(cmake/wisdom.cmake)

# Reflect
CPMAddPackage(
  NAME Reflect
  GITHUB_REPOSITORY qlibs/reflect
  GIT_TAG v1.2.4
)
add_library(Reflect INTERFACE)
target_include_directories(Reflect INTERFACE ${Reflect_SOURCE_DIR})