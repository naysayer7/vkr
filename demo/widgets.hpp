#pragma once
#include "imgui.h"
#include "rtree.h"
#include "state.hpp"
#include "utils.hpp"
#include "renderer.hpp"
#include <string>
#include <algorithm>
#include <cmath>

class Widget
{
public:
    virtual void Show() = 0;
    virtual ~Widget() = default;
};

class ViewportWindow : public Widget
{
private:
    std::string FormatMemorySize(unsigned int bytes)
    {
        const char *suffixes[] = {"B", "KB", "MB", "GB"};
        size_t suffixIndex = 0;
        double count = static_cast<double>(bytes);
        while (count >= 1024.0 && suffixIndex < 3)
        {
            count /= 1024.0;
            ++suffixIndex;
        }
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.2f %s", count, suffixes[suffixIndex]);
        return std::string(buffer);
    }

public:
    void Show() override;

private:
    Camera2D m_Camera;
    DefaultRenderer m_Renderer;
};

void ViewportWindow::Show()
{
    AppState &state = AppState::instance();

    ImGui::BeginChild("Controls", ImVec2(600, 0), true);
    ImGui::Text("Кол-во объектов: %zu", state.GetObjectsCount());
    ImGui::Text("Используемая деревом память: %s", FormatMemorySize(state.GetRTreeMemorySize()).c_str());
    if (state.GetObjectsCount() > 0)
        ImGui::Text("Память на объект: %s", FormatMemorySize(static_cast<unsigned int>((double)state.GetRTreeMemorySize() / state.GetObjectsCount())).c_str());
    else
        ImGui::Text("Память на объект: 0 B");
    ImGui::Separator();
    ImGui::SliderFloat("Масштаб", &m_Camera.zoom, Camera2D::MinZoom(), Camera2D::MaxZoom(), "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Viewport", ImVec2(0, 0), true);
    const ImVec2 requestedSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("viewport_canvas", requestedSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);

    const ImVec2 viewportMin = ImGui::GetItemRectMin();
    const ImVec2 viewportMax = ImGui::GetItemRectMax();

    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    ImGuiIO &io = ImGui::GetIO();
    if (hovered && io.MouseWheel != 0.0f)
    {
        const float scaleFactor = std::pow(1.15f, io.MouseWheel);
        m_Camera.zoom = std::clamp(m_Camera.zoom * scaleFactor, Camera2D::MinZoom(), Camera2D::MaxZoom());
    }

    if (hovered && active && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || ImGui::IsMouseDragging(ImGuiMouseButton_Left)))
    {
        const ImVec2 delta = io.MouseDelta;
        m_Camera.targetWorld.x -= delta.x / m_Camera.zoom;
        m_Camera.targetWorld.y -= delta.y / m_Camera.zoom;
    }

    if (hovered) {
        const ImVec2 mousePos = io.MousePos;
        state.m_MouseWorldPos = m_Camera.ScreenToWorld(mousePos, viewportMin, viewportMax);
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImGui::PushClipRect(viewportMin, viewportMax, true);
    dl->AddRectFilled(viewportMin, viewportMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
    m_Renderer.Render(dl, viewportMin, viewportMax, m_Camera);
    ImGui::PopClipRect();
    ImGui::EndChild();
}
