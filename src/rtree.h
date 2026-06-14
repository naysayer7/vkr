#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <variant>
#include <vector>

namespace rtree {
template <typename T = float>
struct Rectangle {
  std::unique_ptr<T[]> size = nullptr;
  std::size_t n;

  Rectangle(std::size_t n) : n(n) { size = std::make_unique<T[]>(n * 2); }

  Rectangle(const Rectangle& other) : n(other.n) {
    size = std::make_unique<T[]>(n * 2);
    for (std::size_t i = 0; i < n * 2; ++i)
      size[i] = other.size[i];
  }

  Rectangle(Rectangle&& other) noexcept
      : size(std::move(other.size)), n(other.n) {
    other.n = 0;
  }

  Rectangle& operator=(const Rectangle& other) {
    if (this == &other)
      return *this;

    if (n != other.n || !size) {
      size = std::make_unique<T[]>(other.n * 2);
      n = other.n;
    }

    for (std::size_t i = 0; i < n * 2; ++i)
      size[i] = other.size[i];

    return *this;
  }

  Rectangle& operator=(Rectangle&& other) noexcept {
    if (this == &other)
      return *this;

    size = std::move(other.size);
    n = other.n;
    other.n = 0;

    return *this;
  }

  void Zero() {
    for (std::size_t i = 0; i < n * 2; ++i)
      size[i] = T{};
  }

  static Rectangle FromXYWH(T x, T y, T width, T height) {
    Rectangle rect(2);
    rect.size[0] = x;
    rect.size[1] = y;
    rect.size[2] = width;
    rect.size[3] = height;
    return rect;
  }

  bool Intersects(const Rectangle& other) const {
    if (n != other.n)
      throw std::invalid_argument(
          "Прямоугольники должны иметь одинаковое число "
          "измерений для проверки пересечения.");
    for (std::size_t i = 0; i < n; ++i) {
      if (End(i) < other.size[i] || other.End(i) < size[i])
        return false;
    }
    return true;
  }

  Rectangle& Unite(const Rectangle& other) {
    if (n != other.n)
      throw std::invalid_argument(
          "Прямоугольники должны иметь одинаковое число измерений для "
          "объединения.");

    for (std::size_t i = 0; i < n; ++i) {
      const T minv = std::min(size[i], other.size[i]);
      const T maxv = std::max(End(i), other.End(i));
      size[i] = minv;
      size[i + n] = maxv - minv;
    }
    return *this;
  }

  Rectangle Union(const Rectangle& other) const {
    return Rectangle(*this).Unite(other);
  }

  double Volume() const {
    double v = 1.0;
    for (std::size_t i = 0; i < n; ++i) {
      v *= (size[i + n] >= 0) ? size[i + n] : 0;
    }
    return v;
  }

  T MinDistanceSq(const Rectangle& other) const {
    if (n != other.n)
      throw std::invalid_argument(
          "Прямоугольники должны иметь одинаковое число "
          "измерений для вычисления расстояния.");

    T dist = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      T di = 0.0;
      if (End(i) < other.size[i])
        di = other.size[i] - End(i);
      else if (other.End(i) < size[i])
        di = size[i] - other.End(i);
      dist += di * di;
    }
    return dist;
  }

  T End(std::size_t axis) const {
    if (axis >= n)
      throw std::out_of_range(
          "Индекс оси выходит за пределы диапазона в Rectangle::End.");
    return size[axis] + size[axis + n];
  }

  std::size_t MemorySize() const { return sizeof(*this) + BufferMemorySize(); }

  std::size_t BufferMemorySize() const { return n * 2 * sizeof(T); }
};

template <typename T>
struct Object {
  uint64_t id;
  Rectangle<T> mbr;

  Object(uint64_t id, const Rectangle<T>& mbr) : id(id), mbr(mbr) {}

  Object(uint64_t id, Rectangle<T>&& mbr) : id(id), mbr(std::move(mbr)) {}

  std::size_t MemorySize() const {
    return sizeof(*this) + mbr.BufferMemorySize();
  }
};

template <typename T>
struct Node {
  std::vector<const Object<T>*> objects{};
  std::vector<Node<T>*> children{};
  Rectangle<T> mbr;

  Node(std::size_t n) : mbr(n) {}

  bool inline IsLeaf() const { return children.empty(); }

  bool inline IsEmpty() const { return objects.empty() && children.empty(); }

  inline ~Node() {
    for (Node<T>* child : children)
      delete child;
  }

  /**
   * Размер памяти, занимаемой узлом, включая его MBR, объекты и указатели на
   * детей. Не включает память, занимаемую потомками.
   */
  std::size_t MemorySize() const {
    std::size_t size =
        sizeof(*this) +
        mbr.BufferMemorySize();  // Размер самого узла и его MBR. Объекты и дети
                                 // учитываются ниже

    if (objects.empty() && children.empty())
      return size;

    if (!objects.empty())
      size += objects.size() * sizeof(objects.front());
    if (!children.empty())
      size += children.size() * sizeof(children.front());

    return size;
  }
};

/**
 * RTree. Операции удаления нет так как в демонстрации не требуется
 */
template <typename T>
class RTree {
 public:
  using RectangleType = Rectangle<T>;
  using ObjectType = Object<T>;
  using NodeType = Node<T>;

  RTree(std::size_t maxObjectsPerNode,
        std::size_t minObjectsPerNode,
        std::size_t n)
      : maxObjectsPerNode(maxObjectsPerNode),
        minObjectsPerNode(minObjectsPerNode),
        n(n) {
    if (!ValidateParameters(maxObjectsPerNode, minObjectsPerNode))
      throw std::invalid_argument("Недопустимые параметры RTree.");
    root = std::make_unique<NodeType>(n);
  }

  static bool ValidateParameters(std::size_t maxObjectsPerNode,
                                 std::size_t minObjectsPerNode) {
    return minObjectsPerNode <= (maxObjectsPerNode + 1) / 2;
  }

  RTree(const RTree&) = delete;
  RTree& operator=(const RTree&) = delete;

  RTree(RTree&&) noexcept = delete;
  RTree& operator=(RTree&&) noexcept = delete;

  std::size_t GetMaxEntries() const { return maxObjectsPerNode; }
  std::size_t GetMinEntries() const { return minObjectsPerNode; }

  /**
   * ** Дерево не владеет объектами, поэтому объект должет жить не меньше
   * дерева. **
   */
  void Insert(const ObjectType* obj) {
    std::unique_lock lock(treeMutex);
    NodeType* split = InsertRecursive(root.get(), obj);
    if (split) {
      // split — сырой владеющий указатель; сразу берём владение, чтобы он не
      // утёк, если что-то бросит до его привязки к новому корню.
      std::unique_ptr<NodeType> splitOwner(split);
      auto newRoot = std::make_unique<NodeType>(n);
      newRoot->children.reserve(2);
      newRoot->children.push_back(root.release());
      newRoot->children.push_back(splitOwner.release());
      RecomputeMBR(newRoot.get());
      root = std::move(newRoot);
    }
  }

  template <typename Callback>
  void Dfs(Callback&& callback) const {
    std::shared_lock lock(treeMutex);
    if (!root)
      return;
    std::vector<const NodeType*> stack;
    stack.push_back(root.get());

    while (!stack.empty()) {
      const NodeType* current = stack.back();
      stack.pop_back();

      callback(current);

      for (const NodeType* child : current->children)
        stack.push_back(child);
    }
  }

  std::size_t MemorySize() const {
    std::size_t size = 0;
    Dfs([&size](const NodeType* node) {
      if (!node)
        return;
      size += node->MemorySize();
    });
    size += sizeof(*this);
    return size;
  }

  std::size_t GetN() const { return n; }

  template <typename Callback>
  std::vector<const ObjectType*> Search(const RectangleType& area,
                                        Callback&& callback) const {
    std::shared_lock lock(treeMutex);
    std::vector<const ObjectType*> result;
    this->SearchRecursive(root.get(), area, result, callback);
    return result;
  }

  std::vector<const ObjectType*> Search(const RectangleType& area) const {
    return this->Search(area, [](const NodeType*) {});
  }

  // https://scispace.com/pdf/distance-browsing-in-spatial-databases-49rzehxl7r.pdf
  // page 278
  std::vector<const ObjectType*> kNN(const RectangleType& area,
                                     std::size_t k) const {
    struct QueueEntry {
      T dist;
      std::variant<const NodeType*, const ObjectType*> entity;

      bool operator<(const QueueEntry& other) const {
        return dist > other.dist;  // Меньше - выше приоритет
      }
    };

    std::shared_lock lock(treeMutex);

    std::vector<const ObjectType*> result;
    result.reserve(k);
    std::priority_queue<QueueEntry> pq;
    pq.push({0.0, this->root.get()});
    while (!pq.empty() && result.size() < k) {
      auto [dist, entity] = pq.top();
      pq.pop();
      if (std::holds_alternative<const ObjectType*>(entity)) {
        // добавляем объект в результат
        result.push_back(std::get<const ObjectType*>(entity));
      } else {
        const NodeType* node = std::get<const NodeType*>(entity);
        if (node->IsLeaf()) {
          for (const ObjectType* obj : node->objects) {
            T d = obj->mbr.MinDistanceSq(area);
            pq.push({d, obj});
          }
        } else {
          for (const NodeType* child : node->children) {
            T d = child->mbr.MinDistanceSq(area);
            pq.push({d, child});
          }
        }
      }
    }

    return result;
  }

  ~RTree() {
    // Дерево освобождается автоматически, но не объекты
  }

 private:
  std::size_t maxObjectsPerNode;
  std::size_t minObjectsPerNode;
  std::size_t n;
  std::unique_ptr<Node<T>> root;
  mutable std::shared_mutex treeMutex;

  static double Enlargement(const RectangleType& current,
                            const RectangleType& added) {
    const RectangleType u = current.Union(added);
    return u.Volume() - current.Volume();
  }

  static void RecomputeMBR(NodeType* node) {
    if (!node)
      return;

    if (node->IsEmpty()) {
      node->mbr = RectangleType(node->mbr.n);
      return;
    }

    bool first = true;
    for (const NodeType* child : node->children) {
      if (!child)
        continue;
      if (first) {
        node->mbr = child->mbr;
        first = false;
      } else {
        node->mbr.Unite(child->mbr);
      }
    }

    for (const auto& obj : node->objects) {
      if (first) {
        node->mbr = obj->mbr;
        first = false;
      } else {
        node->mbr.Unite(obj->mbr);
      }
    }
  }

  /**
   * Выбирает поддерево для вставки объекта такое, что увеличение объема MBR
   * будет минимальным.
   * @param node Текущий узел.
   * @param rect MBR объекта для вставки.
   */
  NodeType* ChooseSubtree(NodeType* node, const RectangleType& rect) const {
    NodeType* best = node->children[0];
    double bestEnlargement =
        !best->IsEmpty() ? Enlargement(best->mbr, rect) : 0.0;
    double bestVolume = !best->IsEmpty() ? best->mbr.Volume() : 0.0;

    for (NodeType* child : node->children) {
      const double enlargement =
          !child->IsEmpty() ? Enlargement(child->mbr, rect) : 0.0f;
      const double volume = !child->IsEmpty() ? child->mbr.Volume() : 0.0;

      if (enlargement < bestEnlargement ||
          (std::abs(enlargement - bestEnlargement) <=
               std::numeric_limits<double>::epsilon() *
                   std::max({1.0, std::abs(enlargement),
                             std::abs(bestEnlargement)}) &&
           volume < bestVolume)) {
        bestEnlargement = enlargement;
        bestVolume = volume;
        best = child;
      }
    }

    return best;
  }

  /**
   * Рекурсивная вставка объекта в дерево.
   * @param node Текущий узел.
   * @param obj Объект для вставки.
   * @return Указатель на новый узел на том же уровне дерева, если произошло
   * разбиение, иначе nullptr.
   */
  NodeType* InsertRecursive(NodeType* node, const ObjectType* obj) {
    if (node->IsLeaf()) {
      node->objects.push_back(obj);
      if (node->objects.size() <= maxObjectsPerNode) {
        RecomputeMBR(node);
        return nullptr;
      }

      return SplitLeaf(node);
    }

    NodeType* child = ChooseSubtree(node, obj->mbr);
    // ChooseSubtree инициализируется children[0] и никогда не возвращает
    // nullptr
    assert(child != nullptr);

    NodeType* splitChild = InsertRecursive(child, obj);
    if (splitChild) {
      node->children.push_back(splitChild);
      if (node->children.size() > maxObjectsPerNode) {
        return SplitInternal(node);
      }
    }

    RecomputeMBR(node);
    return nullptr;
  }

  using SeedPair = std::pair<std::size_t, std::size_t>;

  /**
   * Линейный алгоритм выбора семян для разбиения узла (Guttman).
   */
  template <typename EntryVec, typename RectFn>
  SeedPair PickSeedsLinear(const EntryVec& entries, RectFn getRect) const {
    T bestSeparation = std::numeric_limits<T>::lowest();
    SeedPair bestSeeds{0, entries.size() > 1 ? 1u : 0u};

    for (std::size_t dim = 0; dim < n; ++dim) {
      // Guttman: найти запись с наибольшим нижним краем и запись с наименьшим
      // верхним краем. Их нормированное расстояние — мера разделённости по оси.
      std::size_t idxMaxLow = 0;
      std::size_t idxMinHigh = 0;
      T maxLow = getRect(entries[0]).size[dim];
      T minHigh = getRect(entries[0]).End(dim);

      T globalMinLow = maxLow;
      T globalMaxHigh = minHigh;

      for (std::size_t i = 1; i < entries.size(); ++i) {
        const auto& r = getRect(entries[i]);
        const T s = r.size[dim];
        const T e = r.End(dim);
        if (s > maxLow) {
          maxLow = s;
          idxMaxLow = i;
        }
        if (e < minHigh) {
          minHigh = e;
          idxMinHigh = i;
        }
        globalMinLow = std::min(globalMinLow, s);
        globalMaxHigh = std::max(globalMaxHigh, e);
      }

      const T width = globalMaxHigh - globalMinLow;
      if (width <= 0.0)
        continue;

      // Отрицательное значение означает перекрытие; выбираем наиболее
      // разделённую пару.
      const T sep = (maxLow - minHigh) / width;
      if (sep > bestSeparation) {
        bestSeparation = sep;
        bestSeeds = SeedPair{idxMaxLow, idxMinHigh};
      }
    }

    if (bestSeeds.first == bestSeeds.second) {
      bestSeeds.first = 0;
      bestSeeds.second = entries.size() > 1 ? 1u : 0u;
    }
    return bestSeeds;
  }

  template <typename Entry>
  struct SplitGroups {
    std::vector<Entry> groupA;
    std::vector<Entry> groupB;
    RectangleType mbrA;
    RectangleType mbrB;
  };

  /**
   * Распределяет записи узла по двум группам линейным алгоритмом Гуттмана:
   * выбирает семена через PickSeedsLinear, затем по очереди добавляет
   * оставшиеся записи в группу с минимальным увеличением MBR, соблюдая
   * минимальную заполненность узла. Общая логика для разбиения листьев и
   * внутренних узлов.
   * @param entries Записи разбиваемого узла (объекты или дочерние узлы).
   * @param getRect Функция, возвращающая MBR записи.
   */
  template <typename Entry, typename RectFn>
  SplitGroups<Entry> SplitEntries(const std::vector<Entry>& entries,
                                  RectFn getRect) const {
    const SeedPair seeds = PickSeedsLinear(entries, getRect);

    std::vector<Entry> groupA;
    std::vector<Entry> groupB;
    groupA.reserve(entries.size());
    groupB.reserve(entries.size());
    groupA.push_back(entries[seeds.first]);
    groupB.push_back(entries[seeds.second]);

    // Mark seeds as used.
    std::vector<bool> used(entries.size(), false);
    used[seeds.first] = true;
    used[seeds.second] = true;

    RectangleType mbrA = getRect(entries[seeds.first]);
    RectangleType mbrB = getRect(entries[seeds.second]);

    const size_t minFill = minObjectsPerNode;

    for (std::size_t remaining = entries.size() - 2; remaining > 0;) {
      // Force-assign if one group must take all remaining to reach min fill.
      if (static_cast<int>(groupA.size()) + static_cast<int>(remaining) ==
          minFill) {
        for (std::size_t i = 0; i < entries.size(); ++i) {
          if (used[i])
            continue;
          used[i] = true;
          --remaining;
          groupA.push_back(entries[i]);
          mbrA.Unite(getRect(entries[i]));
        }
        break;
      }
      if (static_cast<int>(groupB.size()) + static_cast<int>(remaining) ==
          minFill) {
        for (std::size_t i = 0; i < entries.size(); ++i) {
          if (used[i])
            continue;
          used[i] = true;
          --remaining;
          groupB.push_back(entries[i]);
          mbrB.Unite(getRect(entries[i]));
        }
        break;
      }

      // Pick next entry.
      std::size_t pick = 0;
      while (pick < entries.size() && used[pick])
        ++pick;
      if (pick >= entries.size())
        break;

      const RectangleType& r = getRect(entries[pick]);
      const double eA = Enlargement(mbrA, r);
      const double eB = Enlargement(mbrB, r);
      const double vA = mbrA.Volume();
      const double vB = mbrB.Volume();

      bool toA = false;
      if (eA < eB)
        toA = true;
      else if (eB < eA)
        toA = false;
      else if (vA < vB)
        toA = true;
      else if (vB < vA)
        toA = false;
      else
        toA = (groupA.size() <= groupB.size());

      used[pick] = true;
      --remaining;
      if (toA) {
        groupA.push_back(entries[pick]);
        mbrA.Unite(r);
      } else {
        groupB.push_back(entries[pick]);
        mbrB.Unite(r);
      }
    }

    return SplitGroups<Entry>{std::move(groupA), std::move(groupB),
                              std::move(mbrA), std::move(mbrB)};
  }

  NodeType* SplitLeaf(NodeType* node) {
    auto split = SplitEntries(
        node->objects,
        [](const ObjectType* o) -> const RectangleType& { return o->mbr; });

    auto sibling = std::make_unique<NodeType>(n);

    // Дальше только перемещения и clear — без исключений.
    node->objects = std::move(split.groupA);
    sibling->objects = std::move(split.groupB);
    node->children.clear();
    node->mbr = std::move(split.mbrA);
    sibling->mbr = std::move(split.mbrB);
    return sibling.release();
  }

  NodeType* SplitInternal(NodeType* node) {
    auto split = SplitEntries(
        node->children,
        [](const NodeType* nd) -> const RectangleType& { return nd->mbr; });

    auto sibling = std::make_unique<NodeType>(n);

    node->children = std::move(split.groupA);
    sibling->children = std::move(split.groupB);
    node->objects.clear();
    node->mbr = std::move(split.mbrA);
    sibling->mbr = std::move(split.mbrB);
    return sibling.release();
  }

  template <typename Callback>
  void SearchRecursive(const NodeType* node,
                       const RectangleType& area,
                       std::vector<const ObjectType*>& results,
                       Callback& callback) const {
    if (!node)
      return;
    callback(node);

    for (const ObjectType* obj : node->objects) {
      if (obj->mbr.Intersects(area))
        results.push_back(obj);
    }

    for (const NodeType* child : node->children) {
      if (child->mbr.Intersects(area)) {
        SearchRecursive(child, area, results, callback);
      }
    }
  }
};
}  // namespace rtree
