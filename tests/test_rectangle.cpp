#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "rtree.h"

using T = float;
using Rectangle = rtree::Rectangle<T>;
constexpr std::size_t N = 10;

TEST(RectangleTest, BufferMemorySize) {
  Rectangle rect(N);
  EXPECT_EQ(rect.BufferMemorySize(), N * 2 * sizeof(T));
}

TEST(RectangleTest, Copy) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = rect1;  // Copy constructor

  // ASSERT
  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], rect1.size[i]);
}

TEST(RectangleTest, CopyMutation) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = rect1;  // Copy constructor

  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i + 1);

  // ASSERT
  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));
}

TEST(RectangleTest, Move) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = std::move(rect1);  // Move constructor

  // ASSERT
  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));
}

TEST(RectangleTest, MoveNullptr) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = std::move(rect1);  // Move constructor

  EXPECT_EQ(rect1.size, nullptr);
}

TEST(RectangleTest, Zero) {
  Rectangle rect(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect.size[i] = static_cast<T>(i);
  rect.Zero();

  // ASSERT
  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect.size[i], static_cast<T>(0));
}

TEST(RectangleTest, FromXYWH) {
  T x = 1.0f, y = 2.0f, width = 3.0f, height = 4.0f;
  Rectangle rect = Rectangle::FromXYWH(x, y, width, height);

  EXPECT_EQ(rect.n, 2);
  EXPECT_FLOAT_EQ(rect.size[0], x);
  EXPECT_FLOAT_EQ(rect.size[1], y);
  EXPECT_FLOAT_EQ(rect.size[2], width);
  EXPECT_FLOAT_EQ(rect.size[3], height);
}

TEST(RectangleTest, Intersects) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  // ASSERT
  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

TEST(RectangleTest, IntersectsInside) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 1.5f, 1.5f);

  // ASSERT
  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

TEST(RectangleTest, IntersectsTouching) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(2.0f, 1.0f, 1.0f, 1.0f);

  // ASSERT
  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

TEST(RectangleTest, IntersectsSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 3.0f, 1.0f, 1.0f);

  // ASSERT
  EXPECT_FALSE(rect1.Intersects(rect2));
  EXPECT_FALSE(rect2.Intersects(rect1));
}

TEST(RectangleTest, IntersectsDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  // ASSERT
  EXPECT_THROW(rect1.Intersects(rect2), std::invalid_argument);
}

TEST(RectangleTest, Unite) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);
}

TEST(RectangleTest, UniteSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 3.0f, 1.0f, 1.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);
}

TEST(RectangleTest, UniteIdentical) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 2.0f);
  EXPECT_FLOAT_EQ(united.size[3], 2.0f);
}

TEST(RectangleTest, UniteMutatesOriginal) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(rect1.size[0], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[1], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[2], 4.0f);
  EXPECT_FLOAT_EQ(rect1.size[3], 4.0f);
}

TEST(RectangleTest, UniteDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  // ASSERT
  EXPECT_THROW(rect1.Unite(rect2), std::invalid_argument);
}

TEST(RectangleTest, Union) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  Rectangle united = rect1.Union(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);

  // Original rectangles should remain unchanged
  EXPECT_FLOAT_EQ(rect1.size[0], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[1], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[2], 2.0f);
  EXPECT_FLOAT_EQ(rect1.size[3], 2.0f);

  EXPECT_FLOAT_EQ(rect2.size[0], 1.0f);
  EXPECT_FLOAT_EQ(rect2.size[1], 1.0f);
  EXPECT_FLOAT_EQ(rect2.size[2], 3.0f);
  EXPECT_FLOAT_EQ(rect2.size[3], 3.0f);
}

TEST(RectangleTest, UnionDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  // ASSERT
  EXPECT_THROW(rect1.Union(rect2), std::invalid_argument);
}

TEST(RectangleTest, Volume) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 3.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 6.0f);
}

TEST(RectangleTest, VolumeZero) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 0.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 0.0f);
}

TEST(RectangleTest, VolumeNegative) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, -3.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 0.0f);
}

TEST(RectangleTest, MinDistance) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 4.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistance(rect2), 5.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistance(rect1), 5.0f);
}

TEST(RectangleTest, MinDistanceTouching) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(2.0f, 1.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistance(rect2), 0.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistance(rect1), 0.0f);
}

TEST(RectangleTest, MinDistanceSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 4.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistance(rect2), 5.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistance(rect1), 5.0f);
}

TEST(RectangleTest, MinDistanceDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  // ASSERT
  EXPECT_THROW(rect1.MinDistance(rect2), std::invalid_argument);
}

TEST(RectangleTest, End) {
  Rectangle rect = Rectangle::FromXYWH(1.0f, 2.0f, 3.0f, 4.0f);

  EXPECT_FLOAT_EQ(rect.End(0), 4.0f);  // x + width
  EXPECT_FLOAT_EQ(rect.End(1), 6.0f);  // y + height
}

TEST(RectangleTest, EndOutOfRange) {
  Rectangle rect = Rectangle::FromXYWH(1.0f, 2.0f, 3.0f, 4.0f);

  EXPECT_THROW(rect.End(2), std::out_of_range);
  EXPECT_THROW(rect.End(3), std::out_of_range);
}