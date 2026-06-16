#pragma once
#include <chrono>
#include <filesystem>
#include <stdexcept>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"
#include "utils.hpp"

namespace Controllers {

void TestKnnThreadTarget(AppState& state);
inline void Evaluate() {
  AppState& state = AppState::instance();
  state.m_TestKnnState.phase = TestKnnPhase::Progress;
  std::thread thread(TestKnnThreadTarget, std::ref(state));
  thread.detach();
}

std::vector<double> TestKnn(AppState& state);
template <typename T>
inline void TestKnnEpoch(const int& k,
                         const std::vector<rtree::Object<T>>& objects,
                         const rtree::RTree<T>& rtree);
void SaveTestKnnResults(const AppState& state);
inline void TestKnnThreadTarget(AppState& state) {
  try {
    state.m_TestKnnState.progress.Reset();
    state.m_TestKnnState.progress.epochs = state.m_TestKnnState.setup.epochs;

    const auto& maxObjects = state.m_TestKnnState.setup.maxObjects;

    state.m_TestKnnState.progress.runsDone = 0;
    state.m_TestKnnState.progress.runs =
        Utils::CalculateRunsCount(state.m_TestKnnState.setup.maxObjects);

    // STR (bulk-load) не использует нижнюю границу m, поэтому тестируем только по
    // M. Для построения дерева m берётся как максимально допустимое значение
    // (M + 1) / 2 — оно валидно при любом M >= 1 и на STR не влияет.
    for (int M = maxObjects[0]; M <= maxObjects[1]; ++M) {
      const int m = (M + 1) / 2;
      state.m_RTreeParams.maxEntries = M;
      state.m_RTreeParams.minEntries = m;
      state.EnsureRTreeBuiltWithCurrentParameters();
      state.m_TestKnnState.progress.currentParams = state.m_RTreeParams;
      std::vector<double> times = TestKnn(state);
      state.m_TestKnnState.result.times.emplace_back(RTreeParameters{m, M},
                                                     times);
      state.m_TestKnnState.progress.runsDone++;
      state.m_TestKnnState.progress.epochsDone = 0;
    }

    SaveTestKnnResults(state);
    state.m_TestKnnState.phase = TestKnnPhase::Results;
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestKnnState.phase.store(TestKnnPhase::Setup);
  }
}

inline std::vector<double> TestKnn(AppState& state) {
  std::vector<double> times;
  times.reserve(state.m_TestKnnState.setup.epochs);
  const auto tree = state.GetRTree();
  for (size_t i = 0; i < state.m_TestKnnState.setup.epochs; ++i) {
    auto time = Measures::RunMeasure([&]() -> void {
      TestKnnEpoch(state.m_TestKnnState.setup.k, state.m_Objects,
                           *tree);
    });
    times.push_back(time.count());
    state.m_TestKnnState.progress.epochsDone++;
  }
  return times;
}

template <typename T>
inline void TestKnnEpoch(const int& k,
                         const std::vector<rtree::Object<T>>& objects,
                         const rtree::RTree<T>& rtree) {
  for (const auto& obj : objects) {
    rtree.kNN(obj.mbr, k);
  }
}

inline void SaveTestKnnResults(const AppState& state) {
  const std::string resultsDir = std::filesystem::current_path().string() +
                                 "/results/knn/" +
                                 std::to_string(std::time(nullptr));
  if (!std::filesystem::exists(resultsDir)) {
    std::filesystem::create_directory(resultsDir);
  }

  for (const auto& [params, times] : state.m_TestKnnState.result.times) {
    const std::string filename = resultsDir + "/" +
                                 std::to_string(state.m_TestKnnState.setup.k) +
                                 "_" + std::to_string(params.maxEntries) +
                                 ".npy";

    const double* dataPtr = times.data();
    npy::shape_t shape{(npy::ndarray_len_t)times.size()};
    npy::npy_data_ptr<double> data{dataPtr, shape, false};
    std::printf("Saving result file to %s\n", filename.c_str());
    npy::write_npy(filename, data);
  }
}
}  // namespace Controllers