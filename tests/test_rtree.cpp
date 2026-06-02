#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include "rtree.h"

using namespace rtree;
using RectangleType = Rectangle<float>;
using ObjectType = Object<float>;
using RTreeType = RTree<float>;
constexpr std::size_t N = 100;

// Все вставленные объекты присутствуют в листьях дерева ровно по одному разу.
TEST(RTreeTest, InsertCount) {
  const std::size_t numObjects = 100000;
  RTreeType rtree(4, 2, N);

  std::vector<ObjectType> objects;
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

// Конструктор бросает исключение, если m > M/2, и не бросает при корректных параметрах.
TEST(RTreeTest, InvalidConstructorParams) {
  EXPECT_THROW((RTreeType{4, 3, 2}), std::invalid_argument);
  EXPECT_NO_THROW((RTreeType{4, 2, 2}));
}

// Поиск в пустом дереве возвращает пустой результат.
TEST(RTreeTest, SearchEmpty) {
  RTreeType rtree(4, 2, 2);
  auto results = rtree.Search(Rectangle<float>::FromXYWH(0.0f, 0.0f, 10.0f, 10.0f));
  EXPECT_TRUE(results.empty());
}

// Поиск в области, не пересекающей ни один объект, возвращает пустой результат.
TEST(RTreeTest, SearchNoMatch) {
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  objects.emplace_back(0u, Rectangle<float>::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  rtree.Insert(&objects.back());

  auto results = rtree.Search(Rectangle<float>::FromXYWH(5.0f, 5.0f, 1.0f, 1.0f));
  EXPECT_TRUE(results.empty());
}

// Поиск возвращает именно тот объект, чей MBR пересекает область запроса.
TEST(RTreeTest, SearchFindsCorrectObject) {
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  objects.emplace_back(0u, Rectangle<float>::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  objects.emplace_back(1u, Rectangle<float>::FromXYWH(10.0f, 10.0f, 1.0f, 1.0f));
  for (const auto& o : objects)
    rtree.Insert(&o);

  auto results = rtree.Search(Rectangle<float>::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0]->id, 0u);
}

// Поиск по охватывающей области возвращает все вставленные объекты.
TEST(RTreeTest, SearchFindsAll) {
  const int n = 50;
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  for (int i = 0; i < n; ++i)
    objects.emplace_back(static_cast<uint64_t>(i),
                         Rectangle<float>::FromXYWH(i * 2.0f, 0.0f, 1.0f, 1.0f));
  for (const auto& o : objects)
    rtree.Insert(&o);

  auto results = rtree.Search(
      Rectangle<float>::FromXYWH(-1.0f, -1.0f, n * 2.0f + 3.0f, 3.0f));
  EXPECT_EQ(results.size(), static_cast<std::size_t>(n));
}

// Поиск возвращает объекты только из нужного кластера и не захватывает дальний.
TEST(RTreeTest, SearchResultSetCorrect) {
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  for (int i = 0; i < 5; ++i)
    objects.emplace_back(static_cast<uint64_t>(i),
                         Rectangle<float>::FromXYWH(i * 1.0f, 0.0f, 0.5f, 0.5f));
  for (int i = 5; i < 10; ++i)
    objects.emplace_back(static_cast<uint64_t>(i),
                         Rectangle<float>::FromXYWH(100.0f + i * 1.0f, 0.0f, 0.5f, 0.5f));
  for (const auto& o : objects)
    rtree.Insert(&o);

  auto results = rtree.Search(Rectangle<float>::FromXYWH(-1.0f, -1.0f, 10.0f, 2.0f));
  ASSERT_EQ(results.size(), 5u);
  for (const auto* r : results)
    EXPECT_LT(r->id, 5u);
}

// После массовой вставки каждый не-корневой узел содержит от m до M записей включительно.
TEST(RTreeTest, NodeFillConstraint) {
  const std::size_t M = 4, m = 2;
  RTreeType rtree(M, m, 2);
  std::vector<ObjectType> objects;
  for (int i = 0; i < 200; ++i)
    objects.emplace_back(static_cast<uint64_t>(i),
                         Rectangle<float>::FromXYWH(i * 1.0f, 0.0f, 1.0f, 1.0f));
  for (const auto& o : objects)
    rtree.Insert(&o);

  bool isRoot = true;
  rtree.Dfs([&](const RTreeType::NodeType* node) {
    if (isRoot) { isRoot = false; return; }
    const std::size_t entries =
        node->IsLeaf() ? node->objects.size() : node->children.size();
    EXPECT_GE(entries, m);
    EXPECT_LE(entries, M);
  });
}
