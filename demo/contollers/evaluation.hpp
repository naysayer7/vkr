#pragma once
#include <chrono>
#include <stdexcept>

#include "state.hpp"

namespace Controllers {

void EvaluationThreadTarget(EvaluationState& state);
void Evaluate(EvaluationState& state) {
  std::thread thread(EvaluationThreadTarget, std::ref(state));
  state.phase = EvaluationPhase::Progress;
  thread.detach();
}

void EvaluationThreadTarget(EvaluationState& state) {
  std::this_thread::sleep_for(std::chrono::seconds(10));  // Simulate work
  state.phase = EvaluationPhase::Results;
}

}  // namespace Controllers