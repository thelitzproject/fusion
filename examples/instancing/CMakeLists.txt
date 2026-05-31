add_executable(instancing main.cpp)

target_link_libraries(instancing PRIVATE fusion)

add_dependencies(instancing shaders)

add_custom_command(TARGET instancing POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/spirv"
        "$<TARGET_FILE_DIR:instancing>/shaders/spirv"
    COMMENT "Copying shaders to instancing output directory"
)
