#pragma once
#include <string>
#include <cmath>

#include "imgui.h"

struct Scene
{
    const std::vector<rtree::Object<float>> &objects;
    const rtree::RTree<float> &rtree;
    const ImVec2 mouseWorldPos;
};

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
    virtual void Render(ImDrawList *dl, const ImVec2 &viewportMin, const ImVec2 &viewportMax, const Camera2D &camera, const Scene &scene) = 0;
    virtual ~Renderer() = default;
};

class DefaultRenderer : public Renderer
{

    static inline void AddDashedLine(ImDrawList *dl, ImVec2 a, ImVec2 b, ImU32 col, float thickness, float dashLen, float gapLen)
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.0f)
            return;

        const float invLen = 1.0f / len;
        const float ux = dx * invLen;
        const float uy = dy * invLen;

        const float step = dashLen + gapLen;
        if (step <= 0.0f)
            return;

        for (float s = 0.0f; s < len; s += step)
        {
            const float e = (s + dashLen < len) ? (s + dashLen) : len;
            const ImVec2 p0(a.x + ux * s, a.y + uy * s);
            const ImVec2 p1(a.x + ux * e, a.y + uy * e);
            dl->AddLine(p0, p1, col, thickness);
        }
    }

    static inline void AddDashedRect(ImDrawList *dl, ImVec2 p0, ImVec2 p1, ImU32 col, float thickness, float dashLen, float gapLen)
    {
        const ImVec2 a(p0.x, p0.y);
        const ImVec2 b(p1.x, p0.y);
        const ImVec2 c(p1.x, p1.y);
        const ImVec2 d(p0.x, p1.y);
        AddDashedLine(dl, a, b, col, thickness, dashLen, gapLen);
        AddDashedLine(dl, b, c, col, thickness, dashLen, gapLen);
        AddDashedLine(dl, c, d, col, thickness, dashLen, gapLen);
        AddDashedLine(dl, d, a, col, thickness, dashLen, gapLen);
    }

public:
    void Render(
        ImDrawList *dl,
        const ImVec2 &viewportMin,
        const ImVec2 &viewportMax,
        const Camera2D &camera,
        const Scene &scene
    ) override
    {
        if (!dl)
            return;
        if (scene.objects.empty())
        {
            ImGui::PushFont(NULL, 26.0f);
            dl->AddText(
                ImVec2(viewportMin.x + 10.0f, viewportMin.y + 10.0f),
                ImGui::GetColorU32(ImGuiCol_Text),
                "Нет объектов для отображения");
            ImGui::PopFont();
            return;
        }
        if (scene.rtree.GetN() != 2)
        {
            ImGui::PushFont(NULL, 26.0f);
            dl->AddText(
                ImVec2(viewportMin.x + 10.0f, viewportMin.y + 10.0f),
                ImGui::GetColorU32(ImGuiCol_Text),
                ("Размерность объектов(" + std::to_string(scene.rtree.GetN()) + ") не равна 2, отображение не поддерживается").c_str());
            ImGui::PopFont();
            return;
        }
        if (scene.objects.size() > 1000)
        {
            ImGui::PushFont(NULL, 26.0f);
            dl->AddText(
                ImVec2(viewportMin.x + 10.0f, viewportMin.y + 10.0f),
                ImGui::GetColorU32(ImGuiCol_Text),
                "Слишком много объектов для отображения");
            ImGui::PopFont();
            return;
        }

        ImGui::PushFont(NULL, 12.0f * camera.zoom / ImGui::GetIO().FontGlobalScale);
        for (const auto &obj : scene.objects)
        {
            auto x = obj.mbr.size[0];
            auto y = obj.mbr.size[1];
            auto w = obj.mbr.size[2];
            auto h = obj.mbr.size[3];

            const ImVec2 p0 = camera.WorldToScreen(ImVec2(x, y), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(x + w, y + h), viewportMin, viewportMax);
            dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0.0f, 0, 2.0f * camera.zoom);
            dl->AddText(ImVec2(p0.x + 2.0f * camera.zoom, p0.y), ImGui::GetColorU32(ImGuiCol_Text), std::to_string(obj.id).c_str());
        }

        // Tree
        scene.rtree.Dfs([&](const auto *node)
                  {
            if (!node)
                return;
            const float offset = 0.0f;
            auto x = node->mbr.size[0] - offset;
            auto y = node->mbr.size[1] - offset;
            auto w = node->mbr.size[2] + offset * 2;
            auto h = node->mbr.size[3] + offset * 2;
            const ImVec2 p0 = camera.WorldToScreen(ImVec2(x, y), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(x + w, y + h), viewportMin, viewportMax);
            const float thickness = 1.0f * camera.zoom;
            const float dashLen = 6.0f * camera.zoom;
            const float gapLen = 4.0f * camera.zoom;
            AddDashedRect(dl, p0, p1, ImGui::GetColorU32(ImGuiCol_PlotLines), thickness, dashLen, gapLen); });

        // Mouse position
        {
            const ImVec2 mouseWorldPos = scene.mouseWorldPos;
            const ImVec2 p0 = camera.WorldToScreen(ImVec2(mouseWorldPos.x - 5.0f, mouseWorldPos.y - 5.0f), viewportMin, viewportMax);
            const ImVec2 p1 = camera.WorldToScreen(ImVec2(mouseWorldPos.x + 5.0f, mouseWorldPos.y + 5.0f), viewportMin, viewportMax);
            dl->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), ImGui::GetColorU32(ImGuiCol_Text), 2.0f * camera.zoom);
            dl->AddLine(ImVec2(p0.x, p1.y), ImVec2(p1.x, p0.y), ImGui::GetColorU32(ImGuiCol_Text), 2.0f * camera.zoom);
        }

        ImGui::PopFont();
    }
};
