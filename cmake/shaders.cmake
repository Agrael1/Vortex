add_custom_target(shaders)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shaders")

set(SHADER_SOURCES
	"src/vortex/shaders/basic.vs.hlsl"
	"src/vortex/shaders/image.ps.hlsl"
)

target_sources(shaders PRIVATE ${SHADER_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src/vortex FILES ${SHADER_SOURCES})

# Compile shaders
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME_FULL ${SHADER} NAME)
    string(REGEX REPLACE "\\.hlsl$" "" SHADER_NAME "${SHADER_NAME_FULL}")

    WIS_COMPILE_SHADER(
        DXC ${DXC_EXECUTABLE}
        TARGET shaders
        SHADER ${SHADER}
        OUTPUT "${CMAKE_BINARY_DIR}/bin/shaders/${SHADER_NAME}"
        SHADER_MODEL "6.3"
    )
endforeach()