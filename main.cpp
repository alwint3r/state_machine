#include <algorithm> // For std::fill
#include <array>     // For std::array
#include <mdspan>    // For std::mdspan
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

// Use std::array as the underlying storage
static std::array<State, StateSize * EventSize> transitions_storage{};

// Create mdspan view of the transitions storage
static std::mdspan<State, std::extents<size_t, StateSize, EventSize>>
    transitions(transitions_storage.data());

inline void add_transition(State from, State to, Event event) {
  transitions[static_cast<size_t>(from), static_cast<size_t>(event)] = to;
}

inline State transition_to(State currentState, Event event) {
  return transitions[static_cast<size_t>(currentState),
                     static_cast<size_t>(event)];
}

void test_transition(State current, Event event, State expected) {
  State result = transition_to(current, event);
  std::println("State: {}, Event: {} => Result: {}, Expected: {} {}",
               static_cast<int>(current), static_cast<int>(event),
               static_cast<int>(result), static_cast<int>(expected),
               (result == expected) ? "PASS" : "FAIL");
}

int main() {
  // Initialize transitions table with State::MAX_COUNT
  std::fill(transitions_storage.begin(), transitions_storage.end(),
            State::MAX_COUNT);

  add_transition(State::Idle, State::Active, Event::Start);
  add_transition(State::Active, State::Stopped, Event::Timeout);
  add_transition(State::Active, State::Canceled, Event::Cancel);
  add_transition(State::Stopped, State::Active, Event::Restart);

  // Test all defined transitions
  test_transition(State::Idle, Event::Start, State::Active);
  test_transition(State::Active, Event::Timeout, State::Stopped);
  test_transition(State::Active, Event::Cancel, State::Canceled);
  test_transition(State::Stopped, Event::Restart, State::Active);

  // Test some invalid transitions
  test_transition(State::Idle, Event::Timeout, State::MAX_COUNT);
  test_transition(State::Canceled, Event::Restart, State::MAX_COUNT);

  return 0;
}
