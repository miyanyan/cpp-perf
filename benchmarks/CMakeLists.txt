# Helper to add a benchmark executable from <name>.cpp
function(add_benchmark name)
    add_executable(${name} ${name}.cpp)
    target_compile_features(${name} PRIVATE cxx_std_20)
    target_link_libraries(${name} PRIVATE benchmark::benchmark)

    if(MSVC)
        target_compile_options(${name} PRIVATE
            /W4 /permissive- /Zc:__cplusplus /Zc:preprocessor)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_benchmark(string_benchmark)
