#pragma once
#include <filesystem>
#include <thread>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void TestNThreadTarget(AppState& state);

void StartTestN() {
  AppState& state = AppState::instance();
  state.m_TestNState.phase.store(TestNPhase::Progress);
  std::thread t(TestNThreadTarget, std::ref(state));
  t.detach();
}

void TestNThreadTarget(AppState& state) {
  try {
    TestNProgressState& progress = state.m_TestNState.progress;
    const TestNSetupState& setup = state.m_TestNState.setup;

    progress.Reset();

    const int M = setup.maxEntries;
    const int m = setup.minEntries;
    const int step = setup.step;
    const int k = setup.k;
    const int totalObjects = static_cast<int>(state.m_Objects.size());
    const int measurements = setup.CalculateMeasurements(totalObjects);

    if (measurements <= 0)
      throw std::runtime_error(
          "Недостаточно объектов для тестирования: уменьшите шаг.");

    progress.total = measurements;
    progress.epochs = setup.epochs;

    const std::string resultsDir =
        std::filesystem::current_path().string() + "/results/n/" + std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

    const std::size_t dims = state.m_Objects[0].mbr.n;

    for (int i = 1; i <= measurements; ++i) {
      const int n = i * step;
      progress.currentN = n;
      progress.epochsDone = 0;

      // Строим локальное дерево только из первых n объектов,
      // не затрагивая state.m_RTree который используется другими тестами.
      auto tree = std::make_unique<rtree::RTree<float>>(M, m, dims);
      for (int j = 0; j < n; ++j)
        tree->Insert(&state.m_Objects[j]);

      std::vector<double> times;
      times.reserve(setup.epochs);
      for (int e = 0; e < setup.epochs; ++e) {
        auto elapsed = Measures::RunMeasure([&]() {
          for (int j = 0; j < n; ++j)
            tree->kNN(state.m_Objects[j].mbr, k);
        });
        times.push_back(elapsed.count());
        progress.epochsDone++;
      }

      const std::string filename = resultsDir + "/" + std::to_string(n) + "_" +
                                   std::to_string(k) + "_" + std::to_string(M) +
                                   "_" + std::to_string(m) + ".npy";
      npy::npy_data_ptr<double> data{
          times.data(), {(npy::ndarray_len_t)times.size()}, false};
      std::printf("Saving %s\n", filename.c_str());
      npy::write_npy(filename, data);

      progress.done++;
    }

    state.m_TestNState.phase.store(TestNPhase::Results);
  } catch (const std::exception& e) {
    Error::Show(e.what());
    state.m_TestNState.phase.store(TestNPhase::Setup);
  }
}

}  // namespace Controllers
