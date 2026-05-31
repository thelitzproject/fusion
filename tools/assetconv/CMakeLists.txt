add_executable(assetconv main.cpp)

target_include_directories(assetconv PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/stb"
)

target_compile_options(assetconv PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
