add_executable(tests main.cpp)
target_link_libraries(tests PRIVATE GTest::gtest_main EnTT)

include(GoogleTest)
gtest_discover_tests(tests)