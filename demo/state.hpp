#pragma once
#include <mutex>
#include <random>
#include <vector>

#include "renderer.hpp"
#include "rtree.h"

enum class State {
  MainMenu,
  Demo,
  FileReading,
  BuildingRTree,
  Testing,
};

template <typename T>
struct BuildingRTreeState {
  std::size_t totalObjects{0};
  std::size_t handledObjects{0};

  void SetStart() {
    totalObjects = 0;
    handledObjects = 0;
  }

  float GetProgress() const {
    if (totalObjects == 0)
      return 0.0f;
    return static_cast<float>(handledObjects) /
           static_cast<float>(totalObjects);
  }
};

struct DemoState {
  Camera2D m_Camera;
  DefaultRenderer m_Renderer;

  void Reset() { m_Camera = Camera2D(); }
};

class AppState {
  State m_currentState{State::MainMenu};
  std::size_t m_MemorySize{0};
  AppState() {}

 public:
  std::mutex m_Mutex;
  bool m_ShowImGuiDemo = false;
  std::size_t m_ObjSize = 0;

  ImVec2 m_MouseWorldPos{0.0f, 0.0f};
  std::unique_ptr<rtree::RTree<float>> m_RTree = nullptr;
  std::vector<rtree::Object<float>> m_Objects;
  BuildingRTreeState<float> m_BuildingRTreeState;
  DemoState m_DemoState;

  void RecalculateMemorySize() {
    if (m_RTree)
      m_MemorySize = m_RTree->MemorySize();
    else
      m_MemorySize = 0;

    m_ObjSize = 0;
    for (const auto& obj : m_Objects)
      m_ObjSize += obj.MemorySize();
  }

  std::size_t GetRTreeMemorySize() const { return m_MemorySize; }

  size_t GetObjectsCount() const { return m_Objects.size(); }

  void SetStartFileReading() {
    m_RTree.reset();
    m_currentState = State::FileReading;
    m_Objects.clear();
    m_BuildingRTreeState.SetStart();
  }

  void SetCurrentState(State newState) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_currentState = newState;
  }

  State GetCurrentState() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_currentState;
  }

  static AppState& instance() {
    static AppState INSTANCE;
    return INSTANCE;
  }
};