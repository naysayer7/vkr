#pragma once
#include <future>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "measures.hpp"
#include "renderer.hpp"
#include "rtree.h"

enum class State {
  MainMenu,       // Начальное меню
  Demo,           // Демонстрация работы R-дерева
  FileReading,    // Чтение NPY файла
  BuildingRTree,  // Построение R-дерева после чтения файла
  Evaluation,     // Тестирование производительности и точности
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
  Camera2D camera;
  DefaultRenderer renderer;
  bool showObjects = true;
  bool showMBRs = false;
  bool showSearch = false;
  bool showNodeIds = false;
  int kNN = 5;

  void Reset() { camera = Camera2D(); }
};

struct RTreeParameters {
  int maxEntries = 4;
  int minEntries = 2;
};

enum class EvaluationPhase { Setup, Progress, Results };

struct EvaluationResult {
  std::vector<std::pair<RTreeParameters, std::vector<double>>> times;

  void Reset() { times.clear(); }
};

struct EvaluationState {
  EvaluationResult knnResult;
  int numRuns = 1000;
  int k = 5;
  int run = 0;
  EvaluationPhase phase = EvaluationPhase::Setup;

  void Reset() { run = 0; }
};

class AppState {
  State m_currentState{State::MainMenu};
  std::size_t m_MemorySize{0};
  AppState() {}

 public:
  std::mutex m_Mutex;
  RTreeParameters m_RTreeParams;
  bool m_ShowImGuiDemo = false;
  std::size_t m_ObjSize = 0;

  std::vector<std::pair<std::size_t, std::size_t>> m_Params;  // TODO move

  ImVec2 m_MouseWorldPos{0.0f, 0.0f};
  std::unique_ptr<rtree::RTree<float>> m_RTree = nullptr;
  std::vector<rtree::Object<float>> m_Objects;
  BuildingRTreeState<float> m_BuildingRTreeState;
  DemoState m_DemoState;
  EvaluationState m_EvaluationState;

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
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RTree.reset();
    SetCurrentStateUnlocked(State::FileReading);
    m_Objects.clear();
    m_BuildingRTreeState.SetStart();
  }

  void SetCurrentState(State newState) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    SetCurrentStateUnlocked(newState);
  }

  State GetCurrentState() const { return m_currentState; }

  bool IsDemoAvaliable() const {
    if (!m_RTree)
      return false;
    if (m_RTree->GetN() != 2)
      return false;
    if (m_Objects.empty())
      return false;
    if (m_Objects.size() > 15000)
      return false;
    return true;
  }

 private:
  void SetCurrentStateUnlocked(State newState) { m_currentState = newState; }

 public:
  bool IsRTreeBuiltWithCurrentParameters() const {
    if (!m_RTree)
      return false;
    return m_RTree->GetMaxEntries() == m_RTreeParams.maxEntries &&
           m_RTree->GetMinEntries() == m_RTreeParams.minEntries;
  }

  void EnsureRTreeBuiltWithCurrentParameters() {
    if (IsRTreeBuiltWithCurrentParameters())
      return;
    BuildRTree();
  }

  void BuildRTree() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Objects.empty())
      throw std::runtime_error("Cannot build RTree: no objects loaded");
    std::println(
        "Building RTree with {} objects, parameters: maxEntries={}, "
        "minEntries={}",
        m_Objects.size(), m_RTreeParams.maxEntries, m_RTreeParams.minEntries);
    if (m_RTree) {
      std::println("RTree already exists, freeing...");
      m_RTree.reset();
    }

    std::println("Starting to build RTree...");

    m_BuildingRTreeState.totalObjects = m_Objects.size();
    m_BuildingRTreeState.handledObjects = 0;
    State prevState = GetCurrentState();
    SetCurrentStateUnlocked(State::BuildingRTree);
    // Build the R-tree
    m_RTree = std::make_unique<rtree::RTree<float>>(
        m_RTreeParams.maxEntries, m_RTreeParams.minEntries,
        m_Objects.empty() ? 2 : m_Objects[0].mbr.n);
    for (const auto& obj : m_Objects) {
      m_RTree->Insert(&obj);
      ++m_BuildingRTreeState.handledObjects;
    }
    std::println("RTree built successfully.");
    RecalculateMemorySize();
    SetCurrentStateUnlocked(prevState);
  }

  static AppState& instance() {
    static AppState INSTANCE;
    return INSTANCE;
  }
};