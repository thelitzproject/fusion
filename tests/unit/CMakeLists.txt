add_executable(unit_tests
    test_pipeline_cache.cpp
)

target_link_libraries(unit_tests PRIVATE fusion)

add_test(NAME unit_tests COMMAND unit_tests)
