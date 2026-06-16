#pragma once
#include <filesystem>
#include <thread>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void TestKThreadTarget(AppState& state);

inline void StartTestK() {
  AppState& state = AppState::instance();
  state.m_TestKState.phase.store(TestKPhase::Progress);
  std::thread t(TestKThreadTarget, std::ref(state));
  t.detach();
}

inline void TestKThreadTarget(AppState& state) {
  try {
    TestKProgressState& progress = state.m_TestKState.progress;
    const TestKSetupState& setup = state.m_TestKState.setup;

    progress.Reset();

    const int M = setup.maxEntries;
    // STR (bulk-load) не использует нижнюю границу m; берём валидное (M + 1) / 2.
    const int m = (M + 1) / 2;
    const int measurements = setup.CalculateMeasurements();

    if (measurements <= 0)
      throw std::runtime_error(
          "Некорректный диапазон k: проверьте kMin, kMax и шаг.");
    if (state.m_Objects.empty())
      throw std::runtime_error(
          "Невозможно запустить тест: объекты не загружены.");

    progress.total = measurements;
    progress.epochs = setup.epochs;

    const std::string resultsDir = std::filesystem::current_path().string() +
                                   "/results/k/" +
                                   std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

    // Дерево строится один раз — k не влияет на структуру дерева.
    const std::size_t dims = state.m_Objects[0].mbr.n;
    auto tree = std::make_unique<rtree::RTree<double>>(M, m, dims);
    std::vector<const rtree::Object<double>*> objectPtrs;
    objectPtrs.reserve(state.m_Objects.size());
    for (const auto& obj : state.m_Objects)
      objectPtrs.push_back(&obj);
    tree->BulkLoad(std::move(objectPtrs));

    for (int k = setup.kMin; k <= setup.kMax; k += setup.kStep) {
      progress.currentK = k;
      progress.epochsDone = 0;

      std::vector<double> times;
      times.reserve(setup.epochs);
      for (int e = 0; e < setup.epochs; ++e) {
        auto elapsed = Measures::RunMeasure([&]() {
          for (const auto& obj : state.m_Objects)
            tree->kNN(obj.mbr, k);
        });
        times.push_back(elapsed.count());
        progress.epochsDone++;
      }

      const std::string filename =
          resultsDir + "/" + std::to_string(state.m_Objects.size()) + "_" +
          std::to_string(k) + "_" + std::to_string(M) + ".npy";
      npy::npy_data_ptr<double> data{
          times.data(), {(npy::ndarray_len_t)times.size()}, false};
      std::printf("Saving %s\n", filename.c_str());
      npy::write_npy(filename, data);

      progress.done++;
    }

    state.m_TestKState.phase.store(TestKPhase::Results);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestKState.phase.store(TestKPhase::Setup);
  }
}

}  // namespace Controllers
