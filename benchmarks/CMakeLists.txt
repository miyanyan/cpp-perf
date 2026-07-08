file(GLOB_RECURSE bench_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp)

foreach(src IN LISTS bench_sources)
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_compile_features(${name} PRIVATE cxx_std_20)
    target_link_libraries(${name} PRIVATE benchmark::benchmark)

    if(MSVC)
        target_compile_options(${name} PRIVATE
            /W4 /permissive- /Zc:__cplusplus /Zc:preprocessor)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

add_custom_target(run_benchmarks
    COMMAND ${CMAKE_COMMAND} -E echo "Running all benchmarks..."
)
foreach(src IN LISTS bench_sources)
    get_filename_component(name ${src} NAME_WE)
    add_dependencies(run_benchmarks ${name})
    add_custom_command(
        TARGET run_benchmarks
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "-- ${name} --"
        COMMAND $<TARGET_FILE:${name}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()
