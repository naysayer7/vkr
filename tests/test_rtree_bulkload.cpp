#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <random>
#include <set>
#include <vector>

#include "rtree.h"

using namespace rtree;
using RectangleType = Rectangle<float>;
using ObjectType = Object<float>;
using RTreeType = RTree<float>;
using NodeType = RTreeType::NodeType;

namespace {

RectangleType Point(float x, float y) {
  return RectangleType::FromXYWH(x, y, 0.0f, 0.0f);
}

// Собирает указатели на объекты для передачи в BulkLoad.
std::vector<const ObjectType*> Ptrs(const std::vector<ObjectType>& objects) {
  std::vector<const ObjectType*> ptrs;
  ptrs.reserve(objects.size());
  for (const auto& o : objects)
    ptrs.push_back(&o);
  return ptrs;
}

// Возвращает корень дерева: Dfs первым посещает именно его.
const NodeType* Root(const RTreeType& tree) {
  const NodeType* root = nullptr;
  tree.Dfs([&](const NodeType* node) {
    if (!root)
      root = node;
  });
  return root;
}

// k-е по возрастанию значение MinDistanceSq до запроса (брутфорсом).
float KthDistanceSq(const std::vector<ObjectType>& objects,
                    const RectangleType& query,
                    std::size_t k) {
  std::vector<float> dists;
  dists.reserve(objects.size());
  for (const auto& o : objects)
    dists.push_back(o.mbr.MinDistanceSq(query));
  std::sort(dists.begin(), dists.end());
  return dists[std::min(k, dists.size()) - 1];
}

// Генерирует n случайных точек в [0, range]^dims.
std::vector<ObjectType> RandomPoints(std::size_t n,
                                     std::size_t dims,
                                     unsigned seed,
                                     float range = 1000.0f) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> coord(0.0f, range);
  std::vector<ObjectType> objects;
  objects.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    RectangleType rect(dims);
    for (std::size_t d = 0; d < dims; ++d) {
      rect.size[d] = coord(rng);
      rect.size[d + dims] = 0.0f;
    }
    objects.emplace_back(static_cast<uint64_t>(i), rect);
  }
  return objects;
}

}  // namespace

// Пустая загрузка даёт пустое дерево: kNN и Search ничего не возвращают.
TEST(RTreeBulkLoadTest, Empty) {
  RTreeType rtree(10, 4, 2);
  rtree.BulkLoad({});
  EXPECT_TRUE(rtree.kNN(Point(0.0f, 0.0f), 3).empty());
  EXPECT_TRUE(
      rtree.Search(RectangleType::FromXYWH(-1e6f, -1e6f, 2e6f, 2e6f)).empty());
}

// Один объект: дерево из единственного листа, kNN находит его.
TEST(RTreeBulkLoadTest, SingleObject) {
  RTreeType rtree(10, 4, 2);
  std::vector<ObjectType> objects;
  objects.emplace_back(42u, RectangleType::FromXYWH(5.0f, 5.0f, 1.0f, 1.0f));
  rtree.BulkLoad(Ptrs(objects));

  auto results = rtree.kNN(Point(5.0f, 5.0f), 1);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0]->id, 42u);
}

// Все загруженные объекты присутствуют в листьях ровно по одному разу.
TEST(RTreeBulkLoadTest, AllObjectsReachableOnce) {
  const std::size_t n = 100000;
  RTreeType rtree(10, 4, 2);
  auto objects = RandomPoints(n, 2, 1);
  rtree.BulkLoad(Ptrs(objects));

  std::set<uint64_t> seen;
  std::size_t counter = 0;
  rtree.Dfs([&](const NodeType* node) {
    for (const auto* obj : node->objects) {
      seen.insert(obj->id);
      ++counter;
    }
  });
  EXPECT_EQ(counter, n);
  EXPECT_EQ(seen.size(), n);
}

// Ни один узел не превышает вместимость M. Нижняя граница m для STR не
// гарантируется: последний лист/родитель получает остаток и может быть меньше m.
TEST(RTreeBulkLoadTest, CapacityConstraint) {
  const std::size_t M = 10;
  RTreeType rtree(M, 4, 2);
  auto objects = RandomPoints(5000, 2, 2);
  rtree.BulkLoad(Ptrs(objects));

  rtree.Dfs([&](const NodeType* node) {
    EXPECT_LE(node->objects.size(), M);
    EXPECT_LE(node->children.size(), M);
  });
}

// Дерево сбалансировано: все листья на одной глубине.
TEST(RTreeBulkLoadTest, Balanced) {
  RTreeType rtree(10, 4, 2);
  auto objects = RandomPoints(5000, 2, 3);
  rtree.BulkLoad(Ptrs(objects));

  std::set<int> leafDepths;
  std::function<void(const NodeType*, int)> visit = [&](const NodeType* node,
                                                        int depth) {
    if (node->IsLeaf()) {
      leafDepths.insert(depth);
      return;
    }
    for (const auto* child : node->children)
      visit(child, depth + 1);
  };
  visit(Root(rtree), 0);

  EXPECT_EQ(leafDepths.size(), 1u);
}

// Результаты kNN совпадают с полным перебором (по расстоянию до k-го соседа).
TEST(RTreeBulkLoadTest, KNNMatchesBruteForce) {
  const std::size_t k = 7;
  for (unsigned trial = 0; trial < 10; ++trial) {
    auto objects = RandomPoints(2000 + trial * 100, 2, 100 + trial);
    RTreeType rtree(10, 4, 2);
    rtree.BulkLoad(Ptrs(objects));

    std::mt19937 rng(trial);
    for (int q = 0; q < 10; ++q) {
      const auto& query = objects[rng() % objects.size()].mbr;
      auto results = rtree.kNN(query, k);
      ASSERT_EQ(results.size(), k);

      float maxResult = 0.0f;
      for (const auto* o : results)
        maxResult = std::max(maxResult, o->mbr.MinDistanceSq(query));
      EXPECT_NEAR(maxResult, KthDistanceSq(objects, query, k), 1e-2f);
    }
  }
}

// kNN корректен и для многомерных данных.
TEST(RTreeBulkLoadTest, KNNHighDimensional) {
  const std::size_t dims = 5;
  const std::size_t k = 5;
  auto objects = RandomPoints(3000, dims, 7);
  RTreeType rtree(10, 4, dims);
  rtree.BulkLoad(Ptrs(objects));

  std::mt19937 rng(7);
  for (int q = 0; q < 10; ++q) {
    const auto& query = objects[rng() % objects.size()].mbr;
    auto results = rtree.kNN(query, k);
    ASSERT_EQ(results.size(), k);

    float maxResult = 0.0f;
    for (const auto* o : results)
      maxResult = std::max(maxResult, o->mbr.MinDistanceSq(query));
    EXPECT_NEAR(maxResult, KthDistanceSq(objects, query, k), 1e-2f);
  }
}

// Search по охватывающей области возвращает все объекты, по пустой — ничего.
TEST(RTreeBulkLoadTest, SearchMatchesBruteForce) {
  auto objects = RandomPoints(5000, 2, 11);
  RTreeType rtree(10, 4, 2);
  rtree.BulkLoad(Ptrs(objects));

  auto all = rtree.Search(RectangleType::FromXYWH(-1.0f, -1.0f, 2002.0f, 2002.0f));
  EXPECT_EQ(all.size(), objects.size());

  auto none = rtree.Search(RectangleType::FromXYWH(5000.0f, 5000.0f, 1.0f, 1.0f));
  EXPECT_TRUE(none.empty());

  // Окно вокруг конкретной точки должно вернуть её саму.
  const RectangleType& target = objects[123].mbr;
  auto window = rtree.Search(RectangleType::FromXYWH(
      target.size[0] - 0.1f, target.size[1] - 0.1f, 0.2f, 0.2f));
  bool found = std::any_of(window.begin(), window.end(),
                           [](const ObjectType* o) { return o->id == 123u; });
  EXPECT_TRUE(found);
}

// Пять объектов по диагонали: kNN из (0.5,0.5) упорядочен по расстоянию.
TEST(RTreeBulkLoadTest, OrderedByDistance) {
  RTreeType rtree(4, 2, 2);
  std::vector<ObjectType> objects;
  for (int i = 0; i < 5; ++i) {
    float v = static_cast<float>(i * 2);
    objects.emplace_back(static_cast<uint64_t>(i),
                         RectangleType::FromXYWH(v, v, 1.0f, 1.0f));
  }
  rtree.BulkLoad(Ptrs(objects));

  auto results = rtree.kNN(Point(0.5f, 0.5f), 5);
  ASSERT_EQ(results.size(), 5u);
  for (std::size_t i = 0; i < results.size(); ++i)
    EXPECT_EQ(results[i]->id, static_cast<uint64_t>(i));
}

// Объекты с одинаковым MBR оба попадают в результат.
TEST(RTreeBulkLoadTest, OverlappingObjects) {
  RTreeType rtree(10, 4, 2);
  std::vector<ObjectType> objects;
  objects.emplace_back(0u, RectangleType::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  objects.emplace_back(1u, RectangleType::FromXYWH(0.0f, 0.0f, 1.0f, 1.0f));
  rtree.BulkLoad(Ptrs(objects));

  auto results = rtree.kNN(Point(0.5f, 0.5f), 2);
  EXPECT_EQ(results.size(), 2u);
}

// Повторный BulkLoad полностью заменяет содержимое дерева.
TEST(RTreeBulkLoadTest, ReplacesExistingContent) {
  RTreeType rtree(10, 4, 2);

  auto first = RandomPoints(500, 2, 20);
  rtree.BulkLoad(Ptrs(first));

  auto second = RandomPoints(30, 2, 21);
  rtree.BulkLoad(Ptrs(second));

  std::size_t counter = 0;
  rtree.Dfs([&](const NodeType* node) { counter += node->objects.size(); });
  EXPECT_EQ(counter, second.size());
}

// k больше числа объектов — возвращаются все.
TEST(RTreeBulkLoadTest, KLargerThanObjectCount) {
  RTreeType rtree(10, 4, 2);
  auto objects = RandomPoints(5, 2, 30);
  rtree.BulkLoad(Ptrs(objects));

  auto results = rtree.kNN(Point(0.0f, 0.0f), 100);
  EXPECT_EQ(results.size(), 5u);
}
