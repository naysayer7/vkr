#pragma once
#include <future>
#include <chrono>
#include <stdexcept>

#include "state.hpp"

namespace Controllers {

void EvaluationThreadTarget(const EvaluationState& state);
void Evaluate(EvaluationState& state) {
  if (state.isRunning())
    throw std::runtime_error("Evaluation is already running");
  state.futures.clear();
  state.futures.reserve(state.numThreads);
  for (int i = 0; i < state.numThreads; i++) {
    state.futures.emplace_back(
        std::async(std::launch::async, EvaluationThreadTarget, std::cref(state)));
  }
}

void EvaluationThreadTarget(const EvaluationState& state) {
  std::this_thread::sleep_for(std::chrono::seconds(10));  // Simulate work
}

}  // namespace Controllers