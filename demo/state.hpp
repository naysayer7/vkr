#pragma once
#include "rtree.h"
#include <vector>
#include <random>

class AppState
{
    unsigned int m_MemorySize{0};
    AppState()
    {
        const int NUM_OBJECTS = 20;
        m_Objects.reserve(NUM_OBJECTS);
        SDL_Log("Generating %d random objects...", NUM_OBJECTS);

        m_Objects = {
            {0, rtree::Rectangle<2, float>::FromXYWH(0.0f, 0.0f, 20.0f, 80.0f)},
            {1, rtree::Rectangle<2, float>::FromXYWH(40.0f, 0.0f, 20.0f, 80.0f)},
            {2, rtree::Rectangle<2, float>::FromXYWH(-30.0f, 10.0f, 55.0f, 20.0f)},
            {3, rtree::Rectangle<2, float>::FromXYWH(35.0f, 10.0f, 55.0f, 20.0f)},
        };

        /* for (int i = 0; i < NUM_OBJECTS; ++i)
        {
            float x = static_cast<float>((rand() % 2000) - 1000);
            float y = static_cast<float>((rand() % 2000) - 1000);
            float width = static_cast<float>((rand() % 100) + 10);
            float height = static_cast<float>((rand() % 100) + 10);

            rtree::Object<2, float> obj;
            obj.id = i;
            obj.mbr = rtree::Rectangle<2, float>::FromXYWH(x, y, width, height);

            m_Objects.push_back(obj);
        } */

        SDL_Log("Building RTree...");
        for (const auto &obj : m_Objects)
        {
            m_RTree.Insert(obj);
        }
        m_MemorySize = m_RTree.MemorySize();
    }

public:
    ImVec2 m_MouseWorldPos{0.0f, 0.0f};
    rtree::RTree<2, float> m_RTree{2, 1};
    std::vector<rtree::Object<2, float>> m_Objects;
    
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
