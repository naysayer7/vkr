#include <gtest/gtest.h>
#include "rtree.h"

using T = float;
using Rect = rtree::Rectangle<T>;
using Obj = rtree::Object<T>;
using Node = rtree::Node<T>;
using Tree = rtree::RTree<T>;

// ─── Rectangle

// MemorySize равен sizeof структуры плюс размер динамического буфера.
TEST(MemorySizeTest, RectangleMemorySizeEqualsBaseAndBuffer) {
  Rect rect(3);
  EXPECT_EQ(rect.MemorySize(), sizeof(Rect) + rect.BufferMemorySize());
}

// MemorySize растёт линейно с ростом числа измерений.
TEST(MemorySizeTest, RectangleMemorySizeScalesWithDimension) {
  Rect r2(2);
  Rect r4(4);
  EXPECT_EQ(r4.MemorySize() - r2.MemorySize(), (4 - 2) * 2 * sizeof(T));
}

// При n=0 MemorySize равен ровно sizeof структуры без дополнительной памяти.
TEST(MemorySizeTest, RectangleZeroDimensionMemorySize) {
  Rect r0(0);
  EXPECT_EQ(r0.MemorySize(), sizeof(Rect));
  EXPECT_EQ(r0.BufferMemorySize(), 0u);
}

// ─── Object

// MemorySize объекта равен sizeof структуры плюс буфер MBR.
TEST(MemorySizeTest, ObjectMemorySizeEqualsBaseAndBuffer) {
  Rect rect(2);
  Obj obj(0, rect);
  EXPECT_EQ(obj.MemorySize(), sizeof(Obj) + obj.mbr.BufferMemorySize());
}

// MemorySize объекта растёт вместе с размерностью MBR.
TEST(MemorySizeTest, ObjectMemorySizeScalesWithDimension) {
  Obj obj2(1, Rect(2));
  Obj obj5(2, Rect(5));
  EXPECT_EQ(obj5.MemorySize() - obj2.MemorySize(), (5 - 2) * 2 * sizeof(T));
}

// ─── Node

// Пустой листовой узел — MemorySize равен sizeof структуры плюс буфер MBR.
TEST(MemorySizeTest, NodeEmptyLeafMemorySize) {
  Node node(2);
  EXPECT_EQ(node.MemorySize(), sizeof(Node) + node.mbr.BufferMemorySize());
}

// Каждый добавленный указатель на объект увеличивает MemorySize на
// sizeof(указателя).
TEST(MemorySizeTest, NodeMemorySizeGrowsWithObjects) {
  Node node(2);
  const std::size_t baseMem = node.MemorySize();

  Rect r(2);
  Obj obj1(1, r);
  Obj obj2(2, r);
  node.objects.push_back(&obj1);
  node.objects.push_back(&obj2);

  EXPECT_EQ(node.MemorySize(), baseMem + 2 * sizeof(const Obj*));
}

// Каждый дочерний узел увеличивает MemorySize родителя на sizeof(указателя).
// Дочерние узлы создаются через new — деструктор Node удалит их сам.
TEST(MemorySizeTest, NodeMemorySizeGrowsWithChildren) {
  Node parent(2);
  const std::size_t baseMem = parent.MemorySize();

  parent.children.push_back(new Node(2));
  parent.children.push_back(new Node(2));

  EXPECT_EQ(parent.MemorySize(), baseMem + 2 * sizeof(Node*));
}

// ─── RTree

// Пустое дерево занимает ненулевую память (минимум sizeof(RTree) + корневой
// узел).
TEST(MemorySizeTest, RTreeEmptyIsNonZero) {
  Tree tree(4, 2, 2);
  EXPECT_GT(tree.MemorySize(), 0u);
  EXPECT_GE(tree.MemorySize(), sizeof(Tree));
}

// После вставки объектов MemorySize не уменьшается.
TEST(MemorySizeTest, RTreeMemorySizeGrowsAfterInserts) {
  Tree tree(4, 2, 2);
  const std::size_t empty = tree.MemorySize();

  std::vector<Obj> objects;
  objects.reserve(20);
  for (std::size_t i = 0; i < 20; ++i) {
    Rect r(2);
    r.size[0] = static_cast<T>(i);
    r.size[1] = static_cast<T>(i);
    r.size[2] = 1.0f;
    r.size[3] = 1.0f;
    objects.emplace_back(i, r);
    tree.Insert(&objects.back());
  }

  EXPECT_GT(tree.MemorySize(), empty);
}

// MemorySize включает sizeof(RTree) как минимальную базовую составляющую.
TEST(MemorySizeTest, RTreeMemorySizeIncludesBase) {
  Tree tree(4, 2, 2);
  EXPECT_GE(tree.MemorySize(), sizeof(Tree));
}
