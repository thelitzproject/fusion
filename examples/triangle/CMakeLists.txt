add_executable(triangle main.cpp)

target_link_libraries(triangle PRIVATE fusion)

add_dependencies(triangle shaders)

# Copy compiled shaders next to the binary at build time
add_custom_command(TARGET triangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/spirv"
        "$<TARGET_FILE_DIR:triangle>/shaders/spirv"
    COMMENT "Copying shaders to triangle output directory"
)
