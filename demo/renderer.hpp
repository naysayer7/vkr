#pragma once
#include <string>
#include "state.hpp"
#include "imgui.h"

struct Camera2D
{
    ImVec2 targetWorld{0.0f, 0.0f};
    float zoom{1.0f};

    static constexpr float MinZoom() { return 0.2f; }
    static constexpr float MaxZoom() { return 32.0f; }
    
    ImVec2 ScreenToWorld(const ImVec2 &screen, const ImVec2 &viewportMin, const ImVec2 &viewportMax) const
    {
        const ImVec2 viewportCenter{viewportMin.x + (viewportMax.x - viewportMin.x) * 0.5f, viewportMin.y + (viewportMax.y - viewportMin.y) * 0.5f};
        return ImVec2(
            targetWorld.x + (screen.x - viewportCenter.x) / zoom,
            targetWorld.y + (screen.y - viewportCenter.y) / zoom);
    }

    ImVec2 WorldToScreen(const ImVec2 &world, const ImVec2 &viewportMin, const ImVec2 &viewportMax) const
    {
        const ImVec2 viewportCenter{viewportMin.x + (viewportMax.x - viewportMin.x) * 0.5f, viewportMin.y + (viewportMax.y - viewportMin.y) * 0.5f};
        return ImVec2(
            viewportCenter.x + (world.x - targetWorld.x) * zoom,
            viewportCenter.y + (world.y - targetWorld.y) * zoom);
    }
};

class Renderer
{
public:
    virtual void Render(ImDrawList *dl, const ImVec2 &viewportMin, const ImVec2 &viewportMax, const Camera2D &camera) = 0;
    virtual ~Renderer() = default;
};

class DefaultRenderer : public Renderer
{
    AppState &m_AppState = AppState::instance();

public:
    void Render(ImDrawList *dl, const ImVec2 &viewportMin, const ImVec2 &viewportMax, const Camera2D &camera) override
    {
        if (!dl)
            return;
        if (m_AppState.m_Objects.size() > 10000)
            return;

        ImGui::PushFont(NULL, 12.0f * camera.zoom / ImGui::GetIO().FontGlobalScale);
        for (const rtree::Object<2, float> &obj : m_AppState.m_Objects)
        {
            const float x = obj.mbr.start[0];
            const float y = obj.mbr.start[1];
            const float w = obj.mbr.size[0];
            const float h = obj.mbr.size[1];

            const ImVec2 p0 = camera.WorldToScreen(ImVec2(x, y), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(x + w, y + h), viewportMin, viewportMax);
            dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0.0f, 0, 2.0f * camera.zoom);
            dl->AddText(ImVec2(p0.x + 2.0f * camera.zoom, p0.y), ImGui::GetColorU32(ImGuiCol_Text), std::to_string(obj.id).c_str());
        }

        // Tree
        m_AppState.m_RTree.Dfs([&](const rtree::Node<2, float> *node)
        {
            if (!node)
                return;
            const float offset = 0.0f;
            const float x = node->mbr.start[0] - offset;
            const float y = node->mbr.start[1] - offset;
            const float w = node->mbr.size[0] + offset * 2;
            const float h = node->mbr.size[1] + offset * 2;
            const ImVec2 p0 = camera.WorldToScreen(ImVec2(x, y), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(x + w, y + h), viewportMin, viewportMax);
            dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_PlotLines), 0.0f, 0, 1.0f * camera.zoom);
        });

        // Mouse position
        {
            const ImVec2 mouseWorldPos = m_AppState.m_MouseWorldPos;
            const ImVec2 p0 = camera.WorldToScreen(ImVec2(mouseWorldPos.x - 5.0f, mouseWorldPos.y - 5.0f), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(mouseWorldPos.x + 5.0f, mouseWorldPos.y + 5.0f), viewportMin, viewportMax);
            dl->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), ImGui::GetColorU32(ImGuiCol_Text), 2.0f * camera.zoom);
            dl->AddLine(ImVec2(p0.x, p1.y), ImVec2(p1.x, p0.y), ImGui::GetColorU32(ImGuiCol_Text), 2.0f * camera.zoom);
        }

        ImGui::PopFont();
    }
};
