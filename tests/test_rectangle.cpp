#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "rtree.h"

using T = float;
using Rectangle = rtree::Rectangle<T>;
constexpr std::size_t N = 10;

// Размер буфера данных равен n*2 элементам типа T.
TEST(RectangleTest, BufferMemorySize) {
  Rectangle rect(N);
  EXPECT_EQ(rect.BufferMemorySize(), N * 2 * sizeof(T));
}

// Копирующий конструктор создаёт независимую копию всех элементов буфера.
TEST(RectangleTest, Copy) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = rect1;

  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], rect1.size[i]);
}

// Оператор копирующего присваивания создаёт независимую копию буфера.
TEST(RectangleTest, CopyAssignment) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2(N);
  rect2 = rect1;

  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));

  // Изменение оригинала после присваивания не должно влиять на копию.
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = 0.0f;
  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));
}

// Изменение исходного прямоугольника после копирования не затрагивает копию.
TEST(RectangleTest, CopyMutation) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = rect1;

  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i + 1);

  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));
}

// Перемещающий конструктор переносит все данные в новый объект.
TEST(RectangleTest, Move) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = std::move(rect1);

  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect2.size[i], static_cast<T>(i));
}

// После перемещения указатель на буфер исходного объекта равен nullptr.
TEST(RectangleTest, MoveNullptr) {
  Rectangle rect1(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect1.size[i] = static_cast<T>(i);

  Rectangle rect2 = std::move(rect1);

  EXPECT_EQ(rect1.size, nullptr);
}

// Zero обнуляет все элементы буфера.
TEST(RectangleTest, Zero) {
  Rectangle rect(N);
  for (std::size_t i = 0; i < N * 2; i++)
    rect.size[i] = static_cast<T>(i);
  rect.Zero();

  for (std::size_t i = 0; i < N * 2; i++)
    EXPECT_FLOAT_EQ(rect.size[i], static_cast<T>(0));
}

// FromXYWH правильно заполняет поля: первые n — координаты начала, следующие n — размеры.
TEST(RectangleTest, FromXYWH) {
  T x = 1.0f, y = 2.0f, width = 3.0f, height = 4.0f;
  Rectangle rect = Rectangle::FromXYWH(x, y, width, height);

  EXPECT_EQ(rect.n, 2);
  EXPECT_FLOAT_EQ(rect.size[0], x);
  EXPECT_FLOAT_EQ(rect.size[1], y);
  EXPECT_FLOAT_EQ(rect.size[2], width);
  EXPECT_FLOAT_EQ(rect.size[3], height);
}

// Два частично перекрывающихся прямоугольника пересекаются в обоих направлениях.
TEST(RectangleTest, Intersects) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

// Прямоугольник, полностью вложенный в другой, пересекается с ним.
TEST(RectangleTest, IntersectsInside) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 1.5f, 1.5f);

  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

// Прямоугольники, соприкасающиеся по ребру, считаются пересекающимися.
TEST(RectangleTest, IntersectsTouching) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(2.0f, 1.0f, 1.0f, 1.0f);

  EXPECT_TRUE(rect1.Intersects(rect2));
  EXPECT_TRUE(rect2.Intersects(rect1));
}

// Разделённые прямоугольники не пересекаются.
TEST(RectangleTest, IntersectsSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 3.0f, 1.0f, 1.0f);

  EXPECT_FALSE(rect1.Intersects(rect2));
  EXPECT_FALSE(rect2.Intersects(rect1));
}

// Intersects бросает исключение при несовпадении числа измерений.
TEST(RectangleTest, IntersectsDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  EXPECT_THROW(rect1.Intersects(rect2), std::invalid_argument);
}

// Unite расширяет первый прямоугольник до минимального охватывающего обоих.
TEST(RectangleTest, Unite) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);
}

// Unite корректно объединяет разделённые (не перекрывающиеся) прямоугольники.
TEST(RectangleTest, UniteSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 3.0f, 1.0f, 1.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);
}

// Объединение одинаковых прямоугольников не изменяет размеры.
TEST(RectangleTest, UniteIdentical) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);

  Rectangle united = rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 2.0f);
  EXPECT_FLOAT_EQ(united.size[3], 2.0f);
}

// Unite изменяет сам объект, а не только возвращаемое значение.
TEST(RectangleTest, UniteMutatesOriginal) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  rect1.Unite(rect2);

  EXPECT_FLOAT_EQ(rect1.size[0], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[1], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[2], 4.0f);
  EXPECT_FLOAT_EQ(rect1.size[3], 4.0f);
}

// Unite бросает исключение при несовпадении числа измерений.
TEST(RectangleTest, UniteDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  EXPECT_THROW(rect1.Unite(rect2), std::invalid_argument);
}

// Union возвращает объединение, не изменяя исходные прямоугольники.
TEST(RectangleTest, Union) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(1.0f, 1.0f, 3.0f, 3.0f);

  Rectangle united = rect1.Union(rect2);

  EXPECT_FLOAT_EQ(united.size[0], 0.0f);
  EXPECT_FLOAT_EQ(united.size[1], 0.0f);
  EXPECT_FLOAT_EQ(united.size[2], 4.0f);
  EXPECT_FLOAT_EQ(united.size[3], 4.0f);

  EXPECT_FLOAT_EQ(rect1.size[0], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[1], 0.0f);
  EXPECT_FLOAT_EQ(rect1.size[2], 2.0f);
  EXPECT_FLOAT_EQ(rect1.size[3], 2.0f);

  EXPECT_FLOAT_EQ(rect2.size[0], 1.0f);
  EXPECT_FLOAT_EQ(rect2.size[1], 1.0f);
  EXPECT_FLOAT_EQ(rect2.size[2], 3.0f);
  EXPECT_FLOAT_EQ(rect2.size[3], 3.0f);
}

// Union бросает исключение при несовпадении числа измерений.
TEST(RectangleTest, UnionDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  EXPECT_THROW(rect1.Union(rect2), std::invalid_argument);
}

// Volume возвращает произведение размеров по всем осям.
TEST(RectangleTest, Volume) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 3.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 6.0f);
}

// Volume равен нулю, если хотя бы одна сторона равна нулю.
TEST(RectangleTest, VolumeZero) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 0.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 0.0f);
}

// Отрицательный размер трактуется как нулевой при вычислении объёма.
TEST(RectangleTest, VolumeNegative) {
  Rectangle rect = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, -3.0f);
  EXPECT_FLOAT_EQ(rect.Volume(), 0.0f);
}

// MinDistanceSq возвращает сумму квадратов расстояний по каждой оси до ближайшей точки.
TEST(RectangleTest, MinDistanceSq) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 4.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistanceSq(rect2), 5.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistanceSq(rect1), 5.0f);
}

// MinDistanceSq равен нулю для касающихся прямоугольников.
TEST(RectangleTest, MinDistanceTouching) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(2.0f, 1.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistanceSq(rect2), 0.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistanceSq(rect1), 0.0f);
}

// MinDistanceSq симметричен для разделённых прямоугольников.
TEST(RectangleTest, MinDistanceSeparate) {
  Rectangle rect1 = Rectangle::FromXYWH(0.0f, 0.0f, 2.0f, 2.0f);
  Rectangle rect2 = Rectangle::FromXYWH(3.0f, 4.0f, 1.0f, 1.0f);

  EXPECT_FLOAT_EQ(rect1.MinDistanceSq(rect2), 5.0f);
  EXPECT_FLOAT_EQ(rect2.MinDistanceSq(rect1), 5.0f);
}

// MinDistanceSq бросает исключение при несовпадении числа измерений.
TEST(RectangleTest, MinDistanceDifferentDimensions) {
  Rectangle rect1(2);
  Rectangle rect2(3);

  EXPECT_THROW(rect1.MinDistanceSq(rect2), std::invalid_argument);
}

// End возвращает координату правого/нижнего края по заданной оси (начало + размер).
TEST(RectangleTest, End) {
  Rectangle rect = Rectangle::FromXYWH(1.0f, 2.0f, 3.0f, 4.0f);

  EXPECT_FLOAT_EQ(rect.End(0), 4.0f);
  EXPECT_FLOAT_EQ(rect.End(1), 6.0f);
}

// End бросает исключение при выходе индекса оси за пределы n.
TEST(RectangleTest, EndOutOfRange) {
  Rectangle rect = Rectangle::FromXYWH(1.0f, 2.0f, 3.0f, 4.0f);

  EXPECT_THROW(rect.End(2), std::out_of_range);
  EXPECT_THROW(rect.End(3), std::out_of_range);
}
