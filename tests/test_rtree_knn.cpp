#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include "rtree.h"

using namespace rtree;
using RectangleType = Rectangle<float>;
using ObjectType = Object<float>;
using RTreeType = RTree<float>;

static RectangleType Point(float x, float y) {
  return RectangleType::FromXYWH(x, y, 0.0f, 0.0f);
}

// Пять объектов по диагонали: (0,0), (2,2), (4,4), (6,6), (8,8), каждый 1x1.
// Запрос из точки (0.5, 0.5) лежит внутри объекта 0 и удаляется от остальных.
class RTreeKNNTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for (int i = 0; i < 5; ++i) {
      float v = static_cast<float>(i * 2);
      objects.emplace_back(static_cast<uint64_t>(i),
                           RectangleType::FromXYWH(v, v, 1.0f, 1.0f));
    }
    for (const auto& obj : objects)
      rtree.Insert(&obj);
  }

  std::vector<ObjectType> objects;
  RTreeType rtree{4, 2, 2};
};

// kNN возвращает ровно k объектов, когда их в дереве больше k.
TEST_F(RTreeKNNTest, ReturnsKResults) {
  auto results = rtree.kNN(Point(0.5f, 0.5f), 3);
  EXPECT_EQ(results.size(), 3u);
}

// При k=1 возвращается ближайший объект — тот, внутри которого лежит точка запроса.
TEST_F(RTreeKNNTest, NearestIsClosest) {
  auto results = rtree.kNN(Point(0.5f, 0.5f), 1);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0]->id, 0u);
}

// Результаты упорядочены по возрастанию расстояния до точки запроса.
// MinDistanceSq от (0.5, 0.5): 0, 2, 18, 50, 98 — идентификаторы совпадают с порядком.
TEST_F(RTreeKNNTest, OrderedByDistance) {
  auto results = rtree.kNN(Point(0.5f, 0.5f), 5);
  ASSERT_EQ(results.size(), 5u);
  for (std::size_t i = 0; i < results.size(); ++i)
    EXPECT_EQ(results[i]->id, static_cast<uint64_t>(i));
}

// Если k больше числа объектов в дереве, возвращаются все объекты.
TEST_F(RTreeKNNTest, KLargerThanObjectCount) {
  auto results = rtree.kNN(Point(0.5f, 0.5f), 100);
  EXPECT_EQ(results.size(), 5u);
}

// При k=0 результат пуст.
TEST_F(RTreeKNNTest, KZero) {
  auto results = rtree.kNN(Point(0.5f, 0.5f), 0);
  EXPECT_TRUE(results.empty());
}

// kNN на пустом дереве возвращает пустой результат.
TEST(RTreeKNNEmptyTest, EmptyTree) {
  RTreeType rtree(4, 2, 2);
  auto results = rtree.kNN(Point(0.5f, 0.5f), 3);
  EXPECT_TRUE(results.empty());
}

// Два объекта с одинаковым MBR оба попадают в результат при k=2.
TEST(RTreeKNNOverlapTest, OverlappingObjectsAllReturned) {
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  objects.emplace_back(0u, RectangleType::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  objects.emplace_back(1u, RectangleType::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  for (const auto& o : objects)
    rtree.Insert(&o);

  auto results = rtree.kNN(Point(0.5f, 0.5f), 2);
  EXPECT_EQ(results.size(), 2u);
}
