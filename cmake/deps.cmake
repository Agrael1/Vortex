include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(CPM_DONT_UPDATE_MODULE_PATH ON)
set(GET_CPM_FILE "${CMAKE_CURRENT_LIST_DIR}/get_cpm.cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


if (NOT EXISTS ${GET_CPM_FILE})
  file(DOWNLOAD
      https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
      "${GET_CPM_FILE}"
  )
endif()
include(${GET_CPM_FILE})

# Add CPM dependencies here
if (WIN32)
  include(cmake/deps.win32.cmake)
else()
  include(cmake/deps.linux.cmake)
endif()


# spdlog
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog 
  VERSION 1.15.1 
)

# frozen
CPMAddPackage(
  NAME frozen
  GITHUB_REPOSITORY serge-sans-paille/frozen
  GIT_TAG 1.2.0
)

# utfcpp
CPMAddPackage(
  NAME utfcpp
  GITHUB_REPOSITORY nemtrif/utfcpp
  GIT_TAG v4.0.8
)

# DirectXMath
CPMAddPackage(
  NAME DirectXMath
  GITHUB_REPOSITORY microsoft/DirectXMath
  GIT_TAG apr2025
)
add_library(Sal INTERFACE)
add_library(Sal::Sal ALIAS Sal)
target_include_directories(Sal INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(DirectXMath INTERFACE Sal)

install(TARGETS Sal EXPORT SalTargets)
install(EXPORT SalTargets
  FILE SalTargets.cmake
  NAMESPACE Sal::
  DESTINATION lib/cmake/Sal
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