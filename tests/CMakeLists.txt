add_executable(fxmc_tests
    test_models.cpp
    test_simulator.cpp
    test_statistics.cpp
    test_pricing.cpp)
target_link_libraries(fxmc_tests PRIVATE fxmc::fxmc GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(fxmc_tests)
