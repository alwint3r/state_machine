#include <print>

enum class State {
  Idle,
  Active,
  Stopped,
  Canceled,
  MAX_COUNT,
};

// Idle -> Active: Start
// Active -> Stopped: Timeout
// Active -> Canceled: Cancel
// Stopped -> Active: Restart (auto)

enum class Event {
  Start,
  Timeout,
  Cancel,
  Restart,
  MAX_COUNT,
};

static constexpr auto StateSize = static_cast<size_t>(State::MAX_COUNT);
static constexpr auto EventSize = static_cast<size_t>(Event::MAX_COUNT);

// clang-format off
State transitions[StateSize][EventSize] = {
	{State::Active /* Idle, Start -> Active */, State::MAX_COUNT, State::MAX_COUNT, State::MAX_COUNT }, // Idle
	{State::MAX_COUNT, State::Stopped /* Active, Timeout -> Stopped */, State::Canceled, State::MAX_COUNT }, // Active
	{State::MAX_COUNT, State::MAX_COUNT, State::MAX_COUNT, State::Active },  // Stopped
	{State::MAX_COUNT, State::MAX_COUNT, State::MAX_COUNT, State::MAX_COUNT }, // Canceled,
};
// clang-format on

inline State transition_to(State currentState, Event event) {
  return transitions[static_cast<size_t>(currentState),
                     static_cast<size_t>(event)];
}

int main() {
  auto currentState = State::Idle;
  auto nextState = transitions[static_cast<size_t>(currentState)]
                              [static_cast<size_t>(Event::Start)];
  std::println("Current State: {}, Event = {}, NextState = {}",
               static_cast<int>(currentState), static_cast<int>(Event::Start),
               static_cast<int>(nextState));
  return 0;
}
