#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
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

std::vector<double> RunTestKnn(
    AppState& state,
    const rtree::RTree<double>& tree,
    const std::vector<const rtree::Object<double>*>& queries);
template <typename T>
inline void TestKnnEpoch(const int& k,
                         const std::vector<const rtree::Object<T>*>& queries,
                         const rtree::RTree<T>& rtree);
void SaveTestKnnResult(const std::string& resultsDir,
                       const int& k,
                       const std::size_t& queryCount,
                       const std::size_t& indexedCount,
                       const RTreeParameters& params,
                       const std::vector<double>& times);
inline void TestKnnThreadTarget(AppState& state) {
  try {
    state.m_TestKnnState.progress.Reset();
    state.m_TestKnnState.progress.epochs = state.m_TestKnnState.setup.epochs;

    const auto& maxObjects = state.m_TestKnnState.setup.maxObjects;

    state.m_TestKnnState.progress.runsDone = 0;
    state.m_TestKnnState.progress.runs =
        Utils::CalculateRunsCount(state.m_TestKnnState.setup.maxObjects);

    if (state.m_Objects.size() < 2)
      throw std::runtime_error(
          "Невозможно запустить тест: нужно ≥ 2 загруженных объектов для "
          "hold-out.");

    const std::string resultsDir = std::filesystem::current_path().string() +
                                   "/results/knn/" +
                                   std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

    auto split = Utils::SplitHoldout(state.m_Objects,
                                     state.m_TestKnnState.setup.queryPercent,
                                     Utils::kQuerySeed);
    const std::size_t indexedCount = split.indexed.size();
    const std::size_t queryCount = split.queries.size();
    const std::size_t dims = state.m_Objects[0].mbr.n;

    for (int M = maxObjects[0]; M <= maxObjects[1]; ++M) {
      const int m = (M + 1) / 2;
      // Локальное дерево по индексируемому подмножеству (не трогаем общее
      // приложенческое дерево, которое использует Demo и расчёт памяти).
      rtree::RTree<double> tree(M, m, dims);
      tree.BulkLoad(std::vector<const rtree::Object<double>*>(split.indexed));
      state.m_TestKnnState.progress.currentParams = RTreeParameters{m, M};
      std::vector<double> times = RunTestKnn(state, tree, split.queries);
      SaveTestKnnResult(resultsDir, state.m_TestKnnState.setup.k, queryCount,
                        indexedCount, RTreeParameters{m, M}, times);
      state.m_TestKnnState.progress.runsDone++;
      state.m_TestKnnState.progress.epochsDone = 0;
    }

    state.m_TestKnnState.phase = TestKnnPhase::Results;
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestKnnState.phase.store(TestKnnPhase::Setup);
  }
}

inline std::vector<double> RunTestKnn(
    AppState& state,
    const rtree::RTree<double>& tree,
    const std::vector<const rtree::Object<double>*>& queries) {
  std::vector<double> times;
  times.reserve(state.m_TestKnnState.setup.epochs);
  for (size_t i = 0; i < state.m_TestKnnState.setup.epochs; ++i) {
    auto time = Measures::RunMeasure([&]() -> void {
      TestKnnEpoch(state.m_TestKnnState.setup.k, queries, tree);
    });
    times.push_back(time.count());
    state.m_TestKnnState.progress.epochsDone++;
  }
  return times;
}

template <typename T>
inline void TestKnnEpoch(const int& k,
                         const std::vector<const rtree::Object<T>*>& queries,
                         const rtree::RTree<T>& rtree) {
  for (const auto* q : queries) {
    rtree.kNN(q->mbr, k);
  }
}

inline void SaveTestKnnResult(const std::string& resultsDir,
                              const int& k,
                              const std::size_t& queryCount,
                              const std::size_t& indexedCount,
                              const RTreeParameters& params,
                              const std::vector<double>& times) {
  const std::string filename =
      resultsDir + "/indexed=" + std::to_string(indexedCount) +
      "_queries=" + std::to_string(queryCount) + "_k=" + std::to_string(k) +
      "_M=" + std::to_string(params.maxEntries) + ".npy";

  const double* dataPtr = times.data();
  npy::shape_t shape{(npy::ndarray_len_t)times.size()};
  npy::npy_data_ptr<double> data{dataPtr, shape, false};
  std::printf("Saving result file to %s\n", filename.c_str());
  npy::write_npy(filename, data);
}
}  // namespace Controllers