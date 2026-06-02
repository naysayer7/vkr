#pragma once
#include <chrono>
#include <filesystem>
#include <stdexcept>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void EvaluationThreadTarget(AppState& state);
void Evaluate() {
  AppState& state = AppState::instance();
  state.m_EvaluationState.phase = EvaluationPhase::Progress;
  std::thread thread(EvaluationThreadTarget, std::ref(state));
  thread.detach();
}

std::vector<double> Evaluation(AppState& state);
void EvaluationEpoch(const int& k,
                     const std::vector<rtree::Object<float>>& objects,
                     const rtree::RTree<float>& rtree);
void SaveEvaluationResults(const AppState& state);
void EvaluationThreadTarget(AppState& state) {
  try {
    state.m_EvaluationState.progress.Reset();
    state.m_EvaluationState.progress.epochs =
        state.m_EvaluationState.setup.epochs;

    const auto& minObjects = state.m_EvaluationState.setup.minObjects;
    const auto& maxObjects = state.m_EvaluationState.setup.maxObjects;

    state.m_EvaluationState.progress.runsDone = 0;
    state.m_EvaluationState.progress.runs = 0;
    for (int M = maxObjects[0]; M <= maxObjects[1]; ++M) {
      for (int m = minObjects[0]; m <= std::min(minObjects[1], M / 2); ++m) {
        state.m_EvaluationState.progress.runs++;
      }
    }

    for (int M = maxObjects[0]; M <= maxObjects[1]; ++M) {
      for (int m = minObjects[0]; m <= std::min(minObjects[1], M / 2); ++m) {
        state.m_RTreeParams.maxEntries = M;
        state.m_RTreeParams.minEntries = m;
        state.EnsureRTreeBuiltWithCurrentParameters();
        state.m_EvaluationState.progress.currentParams = state.m_RTreeParams;
        std::vector<double> times = Evaluation(state);
        state.m_EvaluationState.result.times.emplace_back(RTreeParameters{M, m},
                                                          times);
        state.m_EvaluationState.progress.runsDone++;
        state.m_EvaluationState.progress.epochsDone = 0;
      }
    }

    SaveEvaluationResults(state);
    state.m_EvaluationState.phase = EvaluationPhase::Results;
  } catch (const std::exception& e) {
    Error::Show(e.what());
    state.m_EvaluationState.phase.store(EvaluationPhase::Setup);
  }
}

std::vector<double> Evaluation(AppState& state) {
  std::vector<double> times;
  times.reserve(state.m_EvaluationState.setup.epochs);
  for (size_t i = 0; i < state.m_EvaluationState.setup.epochs; ++i) {
    auto time = Measures::RunMeasure([&state]() -> void {
      EvaluationEpoch(state.m_EvaluationState.setup.k, state.m_Objects,
                      *state.m_RTree);
    });
    times.push_back(time.count());
    state.m_EvaluationState.progress.epochsDone++;
  }
  return times;
}

void EvaluationEpoch(const int& k,
                     const std::vector<rtree::Object<float>>& objects,
                     const rtree::RTree<float>& rtree) {
  for (const auto& obj : objects) {
    rtree.kNN(obj.mbr, k);
  }
}

void SaveEvaluationResults(const AppState& state) {
  const std::string resultsDir =
      std::filesystem::current_path().string() + "/results";
  if (!std::filesystem::exists(resultsDir)) {
    std::filesystem::create_directory(resultsDir);
  }

  const std::string dirPath =
      resultsDir + "/evaluation_" + std::to_string(std::time(nullptr));

  if (!std::filesystem::exists(dirPath)) {
    std::filesystem::create_directory(dirPath);
  }

  for (const auto& [params, times] : state.m_EvaluationState.result.times) {
    const std::string filename =
        dirPath + "/knn" + std::to_string(state.m_EvaluationState.setup.k) +
        "_" + std::to_string(params.maxEntries) + "_" +
        std::to_string(params.minEntries) + ".npy";

    const double* dataPtr = times.data();
    npy::shape_t shape{(npy::ndarray_len_t)times.size()};
    npy::npy_data_ptr<double> data{dataPtr, shape, false};
    std::printf("Saving result file to %s\n", filename.c_str());
    npy::write_npy(filename, data);
  }
}
}  // namespace Controllers