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
void Evaluate() {
  AppState& state = AppState::instance();
  std::thread thread(EvaluationThreadTarget, std::ref(state));
  state.m_EvaluationState.phase = EvaluationPhase::Progress;
  thread.detach();
}

std::vector<double> Evaluation(AppState& state);
void EvaluationEpoch(const int& k,
                     const std::vector<rtree::Object<float>>& objects,
                     const rtree::RTree<float>& rtree);
void SaveEvaluationResults(const AppState& state);
void EvaluationThreadTarget(AppState& state) {
  state.m_EvaluationState.progress.epochs = state.m_EvaluationState.setup.epochs;
  state.m_EvaluationState.progress.runs = state.m_EvaluationState.setup.params.size();
  state.m_EvaluationState.progress.runsDone = 0;

  const auto& paramsList = state.m_EvaluationState.setup.paramsInput;
  for (const auto& params : state.m_EvaluationState.setup.params) {
    state.m_RTreeParams = params;
    state.EnsureRTreeBuiltWithCurrentParameters();
    std::vector<double> times = Evaluation(state);
    state.m_EvaluationState.result.times.emplace_back(params, times);
    state.m_EvaluationState.progress.runsDone++;
    state.m_EvaluationState.progress.epochsDone = 0;
  }

  SaveEvaluationResults(state);
  state.m_EvaluationState.phase = EvaluationPhase::Results;
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
    auto neighbors = rtree.kNN(obj.mbr, k);
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