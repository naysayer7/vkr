#pragma once
#include <filesystem>
#include <thread>

#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"
#include "utils.hpp"

namespace Controllers {

void TestMemoryThreadTarget(AppState& state);

inline void StartTestMemory() {
  AppState& state = AppState::instance();
  state.m_TestMemoryState.phase.store(TestMemoryPhase::Progress);
  std::thread t(TestMemoryThreadTarget, std::ref(state));
  t.detach();
}

inline void TestMemoryThreadTarget(AppState& state) {
  try {
    TestMemoryProgressState& progress = state.m_TestMemoryState.progress;
    TestMemoryResultState& result = state.m_TestMemoryState.result;
    const TestMemorySetupState& setup = state.m_TestMemoryState.setup;

    result.Reset();
    progress.Reset();

    const auto& minO = setup.minObjects;
    const auto& maxO = setup.maxObjects;

    progress.total = Utils::CalculateRunsCount(minO, maxO);

    for (int M = maxO[0]; M <= maxO[1]; ++M) {
      for (int m = minO[0]; m <= std::min(minO[1], (M + 1) / 2); ++m) {
        state.m_RTreeParams.maxEntries = M;
        state.m_RTreeParams.minEntries = m;
        progress.currentParams = RTreeParameters{m, M};

        state.EnsureRTreeBuiltWithCurrentParameters();
        const std::size_t mem = state.GetRTree()->MemorySize();

        result.memorySizes.emplace_back(RTreeParameters{m, M}, mem);
        progress.done++;
      }
    }

    // Сохранение результатов в results/mem/{time}_{object_count}.npy
    // Формат: 2D массив shape (runs, 3), столбцы: [M, m, memory_bytes]
    const std::string resultsDir =
        std::filesystem::current_path().string() + "/results/mem";
    std::filesystem::create_directories(resultsDir);

    const std::string filename =
        resultsDir + "/" + std::to_string(std::time(nullptr)) + "_" +
        std::to_string(state.GetObjectsCount()) + "_" +
        std::to_string(state.GetRTree()->GetN()) + ".npy";
    result.savedFilename = filename;

    const std::size_t rows = result.memorySizes.size();
    std::vector<size_t> flat;
    flat.reserve(rows * 3);
    for (const auto& [params, mem] : result.memorySizes) {
      flat.push_back(static_cast<size_t>(params.maxEntries));
      flat.push_back(static_cast<size_t>(params.minEntries));
      flat.push_back(static_cast<size_t>(mem));
    }

    npy::npy_data_ptr<size_t> data{
        flat.data(), {(npy::ndarray_len_t)rows, 3}, false};
    std::printf("Saving memory test results to %s\n", filename.c_str());
    npy::write_npy(filename, data);

    state.m_TestMemoryState.phase.store(TestMemoryPhase::Results);
  } catch (const std::exception& e) {
    Error::Show(e.what());
    state.m_TestMemoryState.phase.store(TestMemoryPhase::Setup);
  }
}

}  // namespace Controllers
