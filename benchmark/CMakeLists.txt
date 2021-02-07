find_package(benchmark REQUIRED)

add_executable(zinc_bench main.cc)
enable_cxx_properties_for_target(zinc_bench)
target_link_libraries(zinc_bench libzinc benchmark::benchmark)