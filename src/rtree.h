#pragma once
#include <vector>
#include <functional>
#include <stack>
#include <array>
#include <algorithm>
#include <limits>

namespace rtree
{
    template <std::size_t N, typename T = float>
    struct Rectangle
    {
        static_assert(N > 0, "N must be greater than 0");

        std::array<T, N> start{};
        std::array<T, N> size{};

        template <std::size_t M = N, typename std::enable_if_t<(M == 2), int> = 0>
        static Rectangle FromXYWH(T x, T y, T width, T height)
        {
            Rectangle rect;
            rect.start[0] = x;
            rect.start[1] = y;
            rect.size[0] = width;
            rect.size[1] = height;
            return rect;
        }

        bool Intersects(const Rectangle &other) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (start[i] + size[i] < other.start[i] || other.start[i] + other.size[i] < start[i])
                    return false;
            }
            return true;
        }

        Rectangle &Unite(const Rectangle &other)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                const T minv = std::min(start[i], other.start[i]);
                const T maxv = std::max(End(i), other.End(i));
                start[i] = minv;
                size[i] = maxv - minv;
            }
            return *this;
        }

        Rectangle Union(const Rectangle &other) const
        {
            return Rectangle(*this).Unite(other);
        }

        T Volume() const
        {
            T v = 1.0;
            for (std::size_t i = 0; i < N; ++i)
                v *= (size[i] >= 0) ? size[i] : 0;
            return v;
        }

        T MinDistance(const Rectangle &other) const
        {
            T dist = 0.0;
            for (std::size_t i = 0; i < N; ++i)
            {
                T di = 0.0;
                if (End(i) < other.start[i])
                    di = other.start[i] - End(i);
                else if (other.End(i) < start[i])
                    di = start[i] - other.End(i);
                dist += di * di;
            }
            return dist;
        }

        T End(std::size_t axis) const
        {
            return start[axis] + size[axis];
        }
    };

    template <std::size_t N, typename T>
    struct Object
    {
        int id;
        Rectangle<N, T> mbr{};
    };

    template <std::size_t N, typename T>
    struct Node
    {
        std::vector<Object<N, T>> objects{};
        std::vector<Node<N, T> *> children{};
        Rectangle<N, T> mbr{};

        bool inline IsLeaf() const { return children.empty(); }

        bool inline IsEmpty() const { return objects.empty() && children.empty(); }

        inline ~Node()
        {
            for (Node<N, T> *child : children)
                delete child;
        }
    };

    template <std::size_t N, typename T>
    class RTree
    {
    public:
        using RectangleType = Rectangle<N, T>;
        using ObjectType = Object<N, T>;
        using NodeType = Node<N, T>;

        RTree(std::size_t maxObjectsPerNode, std::size_t minObjectsPerNode)
            : maxObjectsPerNode(maxObjectsPerNode), minObjectsPerNode(minObjectsPerNode)
        {
            root = new NodeType();
        }

        void Insert(const ObjectType &obj)
        {
            NodeType *split = InsertRecursive(root, obj);
            if (split)
            {
                NodeType *newRoot = new NodeType();
                newRoot->children.push_back(root);
                newRoot->children.push_back(split);
                RecomputeMBR(newRoot);
                root = newRoot;
            }
        }

        std::vector<ObjectType> Search(const RectangleType &area) const
        {
            return {};
        }

        void Dfs(std::function<void(const NodeType *)> func) const
        {
            std::vector<const NodeType *> stack;
            stack.push_back(root);

            while (!stack.empty())
            {
                const NodeType *current = stack.back();
                stack.pop_back();

                func(current);

                for (const NodeType *child : current->children)
                    stack.push_back(child);
            }
        }

        unsigned int MemorySize() const
        {
            unsigned int size = 0;
            size += sizeof(*this);

            Dfs([&size](const NodeType *node)
                {
                    size += sizeof(NodeType);
                    size += static_cast<unsigned int>(node->objects.size() * sizeof(ObjectType));
                    size += static_cast<unsigned int>(node->children.size() * sizeof(NodeType *)); });

            return size;
        }

        ~RTree()
        {
            delete root;
        }

    private:
        const std::size_t maxObjectsPerNode;
        const std::size_t minObjectsPerNode;
        Node<N, T> *root;

    private:
        static T Enlargement(const RectangleType &current, const RectangleType &added)
        {
            const RectangleType u = current.Union(added);
            return u.Volume() - current.Volume();
        }

        static void RecomputeMBR(NodeType *node)
        {
            if (!node)
                return;

            if (node->IsEmpty())
            {
                node->mbr = RectangleType{};
                return;
            }

            bool first = true;
            for (const NodeType *child : node->children)
            {
                if (!child)
                    continue;
                if (first)
                {
                    node->mbr = child->mbr;
                    first = false;
                }
                else
                {
                    node->mbr.Unite(child->mbr);
                }
            }

            for (const auto &obj : node->objects)
            {
                if (first)
                {
                    node->mbr = obj.mbr;
                    first = false;
                }
                else
                {
                    node->mbr.Unite(obj.mbr);
                }
            }
        }

        /**
         * Выбирает поддерево для вставки объекта такое, что увеличение объема MBR будет минимальным.
         * @param node Текущий узел.
         * @param rect MBR объекта для вставки.
         */
        NodeType *ChooseSubtree(NodeType *node, const RectangleType &rect) const
        {
            NodeType *best = nullptr;
            T bestEnlargement = std::numeric_limits<T>::infinity();
            T bestVolume = std::numeric_limits<T>::infinity();

            for (NodeType *child : node->children)
            {
                const T enlargement = !child->IsEmpty() ? Enlargement(child->mbr, rect) : 0.0f;
                const T volume = !child->IsEmpty() ? child->mbr.Volume() : 0.0f;

                if (enlargement < bestEnlargement || (abs(enlargement - bestEnlargement) < std::numeric_limits<T>::epsilon() && volume < bestVolume))
                {
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
         * @return Указатель на новый узел на том же уровне дерева, если произошло разбиение, иначе nullptr.
         */
        NodeType *InsertRecursive(NodeType *node, const ObjectType &obj)
        {
            if (node->IsLeaf())
            {
                node->objects.push_back(obj);
                if (node->objects.size() <= maxObjectsPerNode)
                {
                    RecomputeMBR(node);
                    return nullptr;
                }

                return SplitLeaf(node);
            }

            NodeType *child = ChooseSubtree(node, obj.mbr);
            if (!child)
            {
                node->objects.push_back(obj);
                RecomputeMBR(node);
                if (node->objects.size() <= maxObjectsPerNode)
                    return nullptr;
                return SplitLeaf(node);
            }

            NodeType *splitChild = InsertRecursive(child, obj);
            if (splitChild)
            {
                node->children.push_back(splitChild);
                if (node->children.size() > maxObjectsPerNode)
                {
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
        SeedPair PickSeedsLinear(const EntryVec &entries, RectFn getRect) const
        {
            T bestSeparation = -1.0f;
            SeedPair bestSeeds{0, entries.size() > 1 ? 1u : 0u};

            for (std::size_t dim = 0; dim < N; ++dim)
            {
                std::size_t idxMinStart = 0;
                std::size_t idxMaxEnd = 0;
                T minStart = getRect(entries[0]).start[dim];
                T maxEnd = getRect(entries[0]).End(dim);

                T globalMinStart = minStart;
                T globalMaxEnd = maxEnd;

                for (std::size_t i = 1; i < entries.size(); ++i)
                {
                    const RectangleType r = getRect(entries[i]);
                    const T s = r.start[dim];
                    const T e = r.End(dim);
                    if (s < minStart)
                    {
                        minStart = s;
                        idxMinStart = i;
                    }
                    if (e > maxEnd)
                    {
                        maxEnd = e;
                        idxMaxEnd = i;
                    }
                    globalMinStart = std::min(globalMinStart, s);
                    globalMaxEnd = std::max(globalMaxEnd, e);
                }

                const T width = globalMaxEnd - globalMinStart;
                if (width <= 0.0)
                    continue;

                const T sep = (maxEnd - minStart) / width;
                if (sep > bestSeparation)
                {
                    bestSeparation = sep;
                    bestSeeds = SeedPair{idxMinStart, idxMaxEnd};
                }
            }

            if (bestSeeds.first == bestSeeds.second)
            {
                bestSeeds.first = 0;
                bestSeeds.second = entries.size() > 1 ? 1u : 0u;
            }
            return bestSeeds;
        }

        NodeType *SplitLeaf(NodeType *node)
        {
            NodeType *sibling = new NodeType();

            std::vector<ObjectType> entries;
            entries.swap(node->objects);

            auto rectOf = [](const ObjectType &o) -> RectangleType
            { return o.mbr; };
            const SeedPair seeds = PickSeedsLinear(entries, rectOf);

            std::vector<ObjectType> groupA;
            std::vector<ObjectType> groupB;
            groupA.reserve(entries.size());
            groupB.reserve(entries.size());
            groupA.push_back(entries[seeds.first]);
            groupB.push_back(entries[seeds.second]);

            // Mark seeds as used.
            std::vector<bool> used(entries.size(), false);
            used[seeds.first] = true;
            used[seeds.second] = true;

            RectangleType mbrA = groupA[0].mbr;
            RectangleType mbrB = groupB[0].mbr;

            const size_t minFill = minObjectsPerNode;

            for (std::size_t remaining = entries.size() - 2; remaining > 0;)
            {
                // Force-assign if one group must take all remaining to reach min fill.
                if (static_cast<int>(groupA.size()) + static_cast<int>(remaining) == minFill)
                {
                    for (std::size_t i = 0; i < entries.size(); ++i)
                    {
                        if (used[i])
                            continue;
                        used[i] = true;
                        --remaining;
                        groupA.push_back(entries[i]);
                        mbrA.Unite(entries[i].mbr);
                    }
                    break;
                }
                if (static_cast<int>(groupB.size()) + static_cast<int>(remaining) == minFill)
                {
                    for (std::size_t i = 0; i < entries.size(); ++i)
                    {
                        if (used[i])
                            continue;
                        used[i] = true;
                        --remaining;
                        groupB.push_back(entries[i]);
                        mbrB.Unite(entries[i].mbr);
                    }
                    break;
                }

                // Pick next entry.
                std::size_t pick = 0;
                while (pick < entries.size() && used[pick])
                    ++pick;
                if (pick >= entries.size())
                    break;

                const RectangleType r = entries[pick].mbr;
                const T eA = Enlargement(mbrA, r);
                const T eB = Enlargement(mbrB, r);
                const T vA = mbrA.Volume();
                const T vB = mbrB.Volume();

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
                if (toA)
                {
                    groupA.push_back(entries[pick]);
                    mbrA.Unite(r);
                }
                else
                {
                    groupB.push_back(entries[pick]);
                    mbrB.Unite(r);
                }
            }

            node->objects = std::move(groupA);
            sibling->objects = std::move(groupB);
            node->children.clear();
            sibling->children.clear();
            node->mbr = mbrA;
            sibling->mbr = mbrB;
            return sibling;
        }

        NodeType *SplitInternal(NodeType *node)
        {
            NodeType *sibling = new NodeType();

            std::vector<NodeType *> entries;
            entries.swap(node->children);

            auto rectOf = [](const NodeType *n) -> RectangleType
            { return n->mbr; };
            const SeedPair seeds = PickSeedsLinear(entries, rectOf);

            std::vector<NodeType *> groupA;
            std::vector<NodeType *> groupB;
            groupA.reserve(entries.size());
            groupB.reserve(entries.size());
            groupA.push_back(entries[seeds.first]);
            groupB.push_back(entries[seeds.second]);

            std::vector<bool> used(entries.size(), false);
            used[seeds.first] = true;
            used[seeds.second] = true;

            RectangleType mbrA = entries[seeds.first]->mbr;
            RectangleType mbrB = entries[seeds.second]->mbr;

            const size_t minFill = minObjectsPerNode;

            for (std::size_t remaining = entries.size() - 2; remaining > 0;)
            {
                if (static_cast<int>(groupA.size()) + static_cast<int>(remaining) == minFill)
                {
                    for (std::size_t i = 0; i < entries.size(); ++i)
                    {
                        if (used[i])
                            continue;
                        used[i] = true;
                        --remaining;
                        groupA.push_back(entries[i]);
                        mbrA.Unite(entries[i]->mbr);
                    }
                    break;
                }
                if (static_cast<int>(groupB.size()) + static_cast<int>(remaining) == minFill)
                {
                    for (std::size_t i = 0; i < entries.size(); ++i)
                    {
                        if (used[i])
                            continue;
                        used[i] = true;
                        --remaining;
                        groupB.push_back(entries[i]);
                        mbrB.Unite(entries[i]->mbr);
                    }
                    break;
                }

                std::size_t pick = 0;
                while (pick < entries.size() && used[pick])
                    ++pick;
                if (pick >= entries.size())
                    break;

                const RectangleType r = entries[pick]->mbr;
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
                if (toA)
                {
                    groupA.push_back(entries[pick]);
                    mbrA.Unite(r);
                }
                else
                {
                    groupB.push_back(entries[pick]);
                    mbrB.Unite(r);
                }
            }

            node->children = std::move(groupA);
            sibling->children = std::move(groupB);
            node->objects.clear();
            sibling->objects.clear();
            node->mbr = mbrA;
            sibling->mbr = mbrB;
            return sibling;
        }
    };
}
