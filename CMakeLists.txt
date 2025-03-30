cmake_minimum_required (VERSION 3.22)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Vortex")

include(cmake/deps.cmake)

add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE "include")
target_sources(${PROJECT_NAME} 
PRIVATE
  "src/entry_main.cpp"
  

  PRIVATE FILE_SET CXX_MODULES FILES
  "include/util/log.ixx"
  "include/vortex/app.ixx"
  "include/vortex/swapchain.ixx"
 "include/vortex/graphics.ixx")
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  CXX_SCAN_FOR_MODULES ON
)
if (MSVC)
    # Add the /Zc:__cplusplus flag
    target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:__cplusplus")
endif()
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        spdlog::spdlog
        
        wis::wisdom-module
        wis::wisdom-debug-module
        wis::wisdom-extended-allocation-module
        wis::wisdom-descriptor-buffer-module
)

message(STATUS "CMake version: ${CMAKE_VERSION}")