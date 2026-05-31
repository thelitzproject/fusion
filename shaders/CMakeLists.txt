find_program(GLSLC glslc)
if(NOT GLSLC)
    message(WARNING "glslc not found — shader compilation disabled. "
                    "Install the Vulkan SDK or shaderc to enable SPIR-V compilation.")
    add_custom_target(shaders)
    return()
endif()

set(SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spirv")
file(MAKE_DIRECTORY ${SPIRV_DIR})

file(GLOB GLSL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.geom"
)

set(SPIRV_FILES "")
foreach(GLSL ${GLSL_FILES})
    get_filename_component(NAME ${GLSL} NAME)
    set(SPIRV "${SPIRV_DIR}/${NAME}.spv")
    add_custom_command(
        OUTPUT  ${SPIRV}
        COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
        VERBATIM
    )
    list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_FILES})
