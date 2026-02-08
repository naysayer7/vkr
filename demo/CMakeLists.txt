add_executable(rtree_demo main.cpp)
target_link_libraries(rtree_demo PRIVATE rtree_lib SDL3::SDL3-static imgui)
