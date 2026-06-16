#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "measures.hpp"
#include "renderer.hpp"
#include "rtree.h"

enum class State {
  MainMenu,       // Начальное меню
  DemoSetup,      // Настройка параметров дерева перед демо
  Demo,           // Демонстрация работы R-дерева
  FileReading,    // Чтение NPY файла
  BuildingRTree,  // Построение R-дерева после чтения файла
  TestKnn,        // Тестирование производительности KNN-запросов
  TestMemory,     // Тестирование использования памяти
  TestN,          // Тестирование KNN по количеству объектов
  TestK,          // Тестирование KNN по параметру k
};

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
  DefaultRenderer<double> renderer;
  bool showObjects = true;
  bool showMBRs = false;
  bool showSearch = false;
  bool showNodeIds = false;
  int kNN = 5;

  void Reset() { camera = Camera2D(); }
};

struct RTreeParameters {
  int minEntries = 2;
  int maxEntries = 4;
};

enum class TestKnnPhase { Setup, Progress, Results };

enum class TestMemoryPhase { Setup, Progress, Results };

struct TestMemorySetupState {
  int maxObjects[2];  // Диапазон M: [мин, макс]

  void Reset() {
    maxObjects[0] = 2;
    maxObjects[1] = 10;
  }

  TestMemorySetupState() { Reset(); }
};

struct TestMemoryProgressState {
  std::atomic<int> done;
  std::atomic<int> total;
  std::atomic<RTreeParameters> currentParams;

  void Reset() {
    done = 0;
    total = 0;
    currentParams = RTreeParameters{};
  }

  TestMemoryProgressState() { Reset(); }
};

struct TestMemoryResultState {
  std::vector<std::pair<RTreeParameters, std::size_t>> memorySizes;
  std::string savedFilename;
  void Reset() {
    memorySizes.clear();
    savedFilename.clear();
  }
};

struct TestMemoryState {
  std::atomic<TestMemoryPhase> phase{TestMemoryPhase::Setup};
  TestMemorySetupState setup;
  TestMemoryProgressState progress;
  TestMemoryResultState result;

  void Reset() {
    phase.store(TestMemoryPhase::Setup);
    setup = TestMemorySetupState{};
    progress.Reset();
    result.Reset();
  }
};

enum class TestKPhase { Setup, Progress, Results };

struct TestKSetupState {
  int kMin;
  int kMax;
  int kStep;
  int maxEntries;
  int epochs;

  void Reset() {
    kMin = 1;
    kMax = 20;
    kStep = 1;
    maxEntries = 4;
    epochs = 10;
  }

  TestKSetupState() { Reset(); }

  int CalculateMeasurements() const {
    if (kStep <= 0 || kMin > kMax)
      return 0;
    return (kMax - kMin) / kStep + 1;
  }
};

struct TestKProgressState {
  std::atomic<int> done;
  std::atomic<int> total;
  std::atomic<int> epochsDone;
  std::atomic<int> epochs;
  std::atomic<int> currentK;

  void Reset() {
    done = 0;
    total = 0;
    epochsDone = 0;
    epochs = 0;
    currentK = 0;
  }

  TestKProgressState() { Reset(); }
};

struct TestKState {
  std::atomic<TestKPhase> phase{TestKPhase::Setup};
  TestKSetupState setup;
  TestKProgressState progress;

  void Reset() {
    phase.store(TestKPhase::Setup);
    setup = TestKSetupState{};
    progress.Reset();
  }
};

enum class TestNPhase { Setup, Progress, Results };

struct TestNSetupState {
  int minEntries;
  int maxEntries;
  int epochs;
  int k;
  std::vector<std::string> selectedFiles;

  void Reset() {
    minEntries = 2;
    maxEntries = 4;
    epochs = 10;
    k = 5;
    selectedFiles.clear();
  }

  TestNSetupState() { Reset(); }

  int CalculateMeasurements() const {
    return static_cast<int>(selectedFiles.size());
  }
};

struct TestNProgressState {
  std::atomic<int> done;
  std::atomic<int> total;
  std::atomic<int> epochsDone;
  std::atomic<int> epochs;
  std::atomic<int> currentN;

  void Reset() {
    done = 0;
    total = 0;
    epochsDone = 0;
    epochs = 0;
    currentN = 0;
  }

  TestNProgressState() { Reset(); }
};

struct TestNState {
  std::atomic<TestNPhase> phase{TestNPhase::Setup};
  TestNSetupState setup;
  TestNProgressState progress;

  void Reset() {
    phase.store(TestNPhase::Setup);
    setup = TestNSetupState{};
    progress.Reset();
  }
};

struct TestKnnSetupState {
  std::vector<RTreeParameters> params;
  int k;
  int epochs;

  int paramsInput[2];  // min, max

  int maxObjects[2];  // Диапазон M: [мин, макс]

  void Reset() {
    params.clear();
    k = 5;
    epochs = 10;

    paramsInput[0] = 2;
    paramsInput[1] = 4;

    maxObjects[0] = 1;
    maxObjects[1] = 10;
  }

  TestKnnSetupState() { Reset(); }
};

struct TestKnnProgressState {
  std::atomic<int> epochsDone{0};
  std::atomic<int> epochs{0};
  std::atomic<int> runsDone{0};
  std::atomic<int> runs{0};
  std::atomic<RTreeParameters> currentParams{{}};

  void Reset() {
    epochsDone = 0;
    epochs = 0;
    runsDone = 0;
    runs = 0;
    currentParams = RTreeParameters{};
  }

  TestKnnProgressState() { Reset(); }
};

struct TestKnnResultState {
  std::vector<std::pair<RTreeParameters, std::vector<double>>> times;

  void Reset() { times.clear(); }
};

struct TestKnnState {
  std::atomic<TestKnnPhase> phase{TestKnnPhase::Setup};
  TestKnnSetupState setup;
  TestKnnProgressState progress;
  TestKnnResultState result;

  void Reset() {
    phase.store(TestKnnPhase::Setup);
    progress.Reset();
    setup.Reset();
    result.Reset();
  }

  TestKnnState() { Reset(); }
};

class AppState {
  State m_currentState{State::MainMenu};
  std::size_t m_MemorySize{0};

  mutable std::mutex m_RTreeMutex;
  std::shared_ptr<rtree::RTree<double>> m_RTree;

  AppState() {}

  void SetRTree(std::shared_ptr<rtree::RTree<double>> tree) {
    std::lock_guard<std::mutex> lock(m_RTreeMutex);
    m_RTree = std::move(tree);
  }

  void SetCurrentStateUnlocked(State newState) { m_currentState = newState; }

 public:
  std::mutex m_Mutex;
  RTreeParameters m_RTreeParams;
  bool m_ShowImGuiDemo = false;
  std::size_t m_ObjSize = 0;

  ImVec2 m_MouseWorldPos{0.0f, 0.0f};
  std::vector<rtree::Object<double>> m_Objects;
  BuildingRTreeState m_BuildingRTreeState;
  DemoState m_DemoState;
  TestKnnState m_TestKnnState;
  TestMemoryState m_TestMemoryState;
  TestNState m_TestNState;
  TestKState m_TestKState;

  void RecalculateMemorySize() {
    const auto tree = GetRTree();
    m_MemorySize = tree ? tree->MemorySize() : 0;

    m_ObjSize = 0;
    for (const auto& obj : m_Objects)
      m_ObjSize += obj.MemorySize();
  }

  std::size_t GetRTreeMemorySize() const { return m_MemorySize; }

  std::shared_ptr<rtree::RTree<double>> GetRTree() const {
    std::lock_guard<std::mutex> lock(m_RTreeMutex);
    return m_RTree;
  }

  size_t GetObjectsCount() const { return m_Objects.size(); }

  void SetStartFileReading() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    SetRTree(nullptr);
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
    if (m_Objects.empty())
      return false;
    if (m_Objects[0].mbr.n != 2)
      return false;
    if (m_Objects.size() > 15000)
      return false;
    return true;
  }

  bool IsRTreeBuiltWithCurrentParameters() const {
    const auto tree = GetRTree();
    if (!tree)
      return false;
    return tree->GetMaxEntries() == m_RTreeParams.maxEntries &&
           tree->GetMinEntries() == m_RTreeParams.minEntries;
  }

  void EnsureRTreeBuiltWithCurrentParameters() {
    if (IsRTreeBuiltWithCurrentParameters())
      return;
    BuildRTree();
  }

  void BuildRTree(bool bulkLoad = true) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Objects.empty())
      throw std::runtime_error(
          "Невозможно построить R-дерево: объекты не загружены.");
    std::println(
        "Building RTree with {} objects, parameters: maxEntries={}, "
        "minEntries={}",
        m_Objects.size(), m_RTreeParams.maxEntries, m_RTreeParams.minEntries);

    m_BuildingRTreeState.totalObjects = m_Objects.size();
    m_BuildingRTreeState.handledObjects = 0;
    State prevState = GetCurrentState();
    SetCurrentStateUnlocked(State::BuildingRTree);

    std::println("Starting to build RTree...");

    auto tree = std::make_shared<rtree::RTree<double>>(
        m_RTreeParams.maxEntries, m_RTreeParams.minEntries, m_Objects[0].mbr.n);
    if (bulkLoad) {
      std::vector<const rtree::Object<double>*> objectPtrs;
      objectPtrs.reserve(m_Objects.size());
      for (const auto& obj : m_Objects)
        objectPtrs.push_back(&obj);
      tree->BulkLoad(std::move(objectPtrs));
      m_BuildingRTreeState.handledObjects = m_Objects.size();
    } else {
      for (const auto& obj : m_Objects) {
        tree->Insert(&obj);
        ++m_BuildingRTreeState.handledObjects;
      }
    }
    SetRTree(std::move(tree));
    std::println("RTree built successfully.");
    RecalculateMemorySize();
    SetCurrentStateUnlocked(prevState);
  }

  static AppState& instance() {
    static AppState INSTANCE;
    return INSTANCE;
  }
};