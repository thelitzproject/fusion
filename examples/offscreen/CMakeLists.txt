add_executable(offscreen main.cpp)

target_link_libraries(offscreen PRIVATE fusion)

add_dependencies(offscreen shaders)

add_custom_command(TARGET offscreen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/spirv"
        "$<TARGET_FILE_DIR:offscreen>/shaders/spirv"
    COMMENT "Copying shaders to offscreen output directory"
)
