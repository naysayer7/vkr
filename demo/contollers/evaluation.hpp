#pragma once
#include <chrono>
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

void EvaluationEpoch(AppState& state);
void EvaluationThreadTarget(AppState& state) {
  std::vector<Measures::Duration> times;
  times.reserve(state.m_EvaluationState.numRuns);
  Measures::Duration totalTime = Measures::Duration(0);
  for (size_t i = 0; i < state.m_EvaluationState.numRuns; ++i) {
    auto time =
        Measures::RunMeasure([&state]() -> void { EvaluationEpoch(state); });
    times.push_back(time);
    totalTime += time;
    state.m_EvaluationState.run++;
    state.m_EvaluationState.knnResult.averageTime = totalTime / (i + 1);
  }

  state.m_EvaluationState.knnResult.averageTime =
      totalTime / state.m_EvaluationState.numRuns;
  state.m_EvaluationState.phase = EvaluationPhase::Results;
}

void EvaluationEpoch(AppState& state) {
  const auto& k = state.m_EvaluationState.k;
  for (const auto& obj : state.m_Objects) {
    auto neighbors = state.m_RTree->kNN(obj.mbr, k);
  }
}

void SaveEvaluationResults(const AppState& state) {
  const std::string filename = "evaluation_results.npy";

  npy::write_npy(filename, npy::npy_data_ptr<double>{
                               state.m_EvaluationState.knnResult.times.data(),
                               {state.m_EvaluationState.knnResult.times.size()},
                               false});
}
}  // namespace Controllers