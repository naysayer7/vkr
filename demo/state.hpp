#pragma once
#include <vector>
#include <random>

#include "rtree.h"

template <typename T>
struct FileLoadingState {
    bool loading = false;
    bool fileRead = false;
    std::size_t totalLines = 0;
    std::size_t linesRead = 0;
    std::size_t objectsInserted = 0;
    std::string errorMessage = "";

    void SetStartReading() {
        loading = true;
        fileRead = false;
        totalLines = 0;
        linesRead = 0;
        objectsInserted = 0;
        errorMessage = "";
    }
};

class AppState
{
    std::size_t m_MemorySize{0};
    AppState()
    {
        m_Objects = {
            /* {0, rtree::Rectangle<float>::FromXYWH(0.0f, 0.0f, 20.0f, 80.0f)},
            {1, rtree::Rectangle<float>::FromXYWH(40.0f, 0.0f, 20.0f, 80.0f)},
            {2, rtree::Rectangle<float>::FromXYWH(-30.0f, 10.0f, 55.0f, 20.0f)},
            {3, rtree::Rectangle<float>::FromXYWH(35.0f, 10.0f, 55.0f, 20.0f)},
            {4, rtree::Rectangle<float>::FromXYWH(85.0f, 40.0f, 55.0f, 20.0f)},
            {5, rtree::Rectangle<float>::FromXYWH(125.0f, 80.0f, 55.0f, 20.0f)}, */
        };

        for (const auto &obj : m_Objects)
            m_RTree.Insert(obj);
        m_MemorySize = m_RTree.MemorySize();
    }

public:
    ImVec2 m_MouseWorldPos{0.0f, 0.0f};
    rtree::RTree<float> m_RTree{4, 2, 2};
    std::vector<rtree::Object<float>> m_Objects;
    FileLoadingState<float> m_FileLoadingState;
    
    void RecalculateMemorySize()
    {
        m_MemorySize = m_RTree.MemorySize();
    }

    unsigned int GetRTreeMemorySize() const
    {
        return m_MemorySize;
    }

    size_t GetObjectsCount() const
    {
        return m_Objects.size();
    }

    static AppState &instance()
    {
        static AppState INSTANCE;
        return INSTANCE;
    }
};