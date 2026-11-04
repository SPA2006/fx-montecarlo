add_executable(fxmc_bench bench_fxmc.cpp)
target_link_libraries(fxmc_bench PRIVATE fxmc::fxmc benchmark::benchmark_main)
