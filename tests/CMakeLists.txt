add_executable(rtree_tests test_rtree.cpp test_rectangle.cpp)
target_link_libraries(rtree_tests PRIVATE rtree_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(rtree_tests)
