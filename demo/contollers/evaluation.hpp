#pragma once
#include <chrono>
#include <stdexcept>

#include "state.hpp"
#include "../measures.hpp"

namespace Controllers {

void EvaluationThreadTarget(EvaluationState& state);
void Evaluate(EvaluationState& state) {
  std::thread thread(EvaluationThreadTarget, std::ref(state));
  state.phase = EvaluationPhase::Progress;
  thread.detach();
}

void EvaluationThreadTarget(EvaluationState& state) {
  auto result = Measures::RunMeasure([]() -> void {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulate work
  }, 1000, [&state](const size_t& iteration,
    const Measures::Duration& time,
    const Measures::Duration& totalTime) -> void {
      state.run++;
  });

  state.knnResult.averangeTime = result.averageTime;
  state.phase = EvaluationPhase::Results;
}

}  // namespace Controllers