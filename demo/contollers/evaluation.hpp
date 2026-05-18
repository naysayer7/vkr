#pragma once
#include <chrono>
#include <filesystem>
#include <stdexcept>


#include "../measures.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void EvaluationThreadTarget(AppState& state);
void Evaluate(AppState& state) {
  std::thread thread(EvaluationThreadTarget, std::ref(state));
  state.m_EvaluationState.phase = EvaluationPhase::Progress;
  thread.detach();
}

std::vector<double> Evaluation(AppState& state);
void EvaluationEpoch(AppState& state);
void SaveEvaluationResults(const AppState& state);
void EvaluationThreadTarget(AppState& state) {
  for (const std::pair<std::size_t, std::size_t>& params : state.m_Params) {
    RTreeParameters rtreeParams{(int)params.first, (int)params.second};
    state.m_RTreeParams = rtreeParams;
    state.EnsureRTreeBuiltWithCurrentParameters();
    std::vector<double> times = Evaluation(state);
    state.m_EvaluationState.knnResult.times.emplace_back(state.m_RTreeParams,
                                                         times);
  }

  Evaluation(state);
  SaveEvaluationResults(state);
  state.m_EvaluationState.phase = EvaluationPhase::Results;
}

std::vector<double> Evaluation(AppState& state) {
  std::vector<double> times;
  times.reserve(state.m_EvaluationState.numRuns);
  for (size_t i = 0; i < state.m_EvaluationState.numRuns; ++i) {
    auto time =
        Measures::RunMeasure([&state]() -> void { EvaluationEpoch(state); });
    times.push_back(time.count());
    state.m_EvaluationState.run++;
  }
  return times;
}

void EvaluationEpoch(AppState& state) {
  const auto& k = state.m_EvaluationState.k;
  for (const auto& obj : state.m_Objects) {
    auto neighbors = state.m_RTree->kNN(obj.mbr, k);
  }
}

void SaveEvaluationResults(const AppState& state) {
  const std::string resultsDir = std::filesystem::current_path().string() + "/results";
  if (!std::filesystem::exists(resultsDir)) {
    std::filesystem::create_directory(resultsDir);
  }
  
  const std::string dirPath =
      resultsDir + "/evaluation_" + std::to_string(std::time(nullptr));

  if (!std::filesystem::exists(dirPath)) {
    std::filesystem::create_directory(dirPath);
  }

  for (const auto& [params, times] : state.m_EvaluationState.knnResult.times) {
    const std::string filename = dirPath + "/knn" +
                                 std::to_string(state.m_EvaluationState.k) +
                                 "_" + std::to_string(params.maxEntries) + "_" +
                                 std::to_string(params.minEntries) + ".npy";

    const double* dataPtr = times.data();
    npy::shape_t shape{(npy::ndarray_len_t)times.size()};
    npy::npy_data_ptr<double> data{dataPtr, shape, false};
    std::printf("Saving result file to %s\n", filename);
    npy::write_npy(filename, data);
  }
}
}  // namespace Controllers