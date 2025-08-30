project(vortex_tests)

include(CTest)
include(Catch)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PRIVATE
	"test_model.cpp"
 "mock_output.h" "test_graph.cpp" "test_byte_ring.cpp" "mock_model.h")
WIS_INSTALL_DEPS(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE VortexLib Catch2::Catch2WithMain)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
# copy all dlls to the output directory
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
  )
  FFMPEG_COPY_DLL(${PROJECT_NAME})
endif()

# Add the shaders target as a dependency
add_dependencies(${PROJECT_NAME} shaders)
# Copy the shaders to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_BINARY_DIR}/bin/shaders"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)


catch_discover_tests(${PROJECT_NAME})