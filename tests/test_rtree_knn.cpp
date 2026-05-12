#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include "rtree.h"

using namespace rtree;
using RectangleType = Rectangle<float>;
using ObjectType = Object<float>;
using RTreeType = RTree<float>;

TEST(RTreeKNNTest, Simple) {
  RTreeType rtree(4, 2, 2);

  std::vector<RectangleType> rectangles = {
      RectangleType::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f),
      RectangleType::FromXYWH(2.0f, 2.0f, 1.0f, 1.0f),
      RectangleType::FromXYWH(4.0f, 4.0f, 1.0f, 1.0f),
      RectangleType::FromXYWH(6.0f, 6.0f, 1.0f, 1.0f),
      RectangleType::FromXYWH(8.0f, 8.0f, 1.0f, 1.0f),
  };

  std::vector<ObjectType> objects {
    
  };
  objects.reserve(numObjects);
  for (size_t i = 0; i < numObjects; ++i) {
    RectangleType rect(N);
    for (size_t j = 0; j < N * 2; ++j)
      rect.size[j] = static_cast<float>(i + j);
    objects.emplace_back(i, rect);
    rtree.Insert(&objects.back());
  }

  std::size_t counter = 0;
  rtree.Dfs([&objects, &counter](const typename RTreeType::NodeType* node) {
    if (!node)
      return;
    for (const auto& obj : node->objects) {
      auto it =
          std::find_if(objects.begin(), objects.end(),
                       [&obj](const ObjectType& o) { return o.id == obj->id; });
      EXPECT_NE(it, objects.end());
      counter++;
    }
  });

  EXPECT_EQ(counter, numObjects);
}