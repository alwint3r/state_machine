#include <algorithm> // For std::fill
#include <array>     // For std::array
#include <functional>
#include <mdspan> // For std::mdspan
#include <print>
#include <unordered_map>

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
// Stopped -> Active: Restart

enum class Event {
  Start,
  Timeout,
  Cancel,
  Restart,
  MAX_COUNT,
};

static constexpr auto StateSize = static_cast<size_t>(State::MAX_COUNT);
static constexpr auto EventSize = static_cast<size_t>(Event::MAX_COUNT);

static std::array<State, StateSize * EventSize> transitions_storage{};
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

enum class TransitionType {
  Before,
  After,
};

struct TransitionCallbackKey {
  TransitionType type;
  State state;
};

struct TransitionCallbackKeyHash {
  size_t operator()(const TransitionCallbackKey &key) const noexcept {
    size_t h1 = std::hash<int>{}(static_cast<int>(key.type));
    size_t h2 = std::hash<int>{}(static_cast<int>(key.state));
    return h1 ^ (h2 << 1);
  }
};

inline bool operator==(const TransitionCallbackKey &lhs,
                       const TransitionCallbackKey &rhs) {
  return lhs.type == rhs.type && lhs.state == rhs.state;
}

using TransitionsCallback =
    std::function<void(TransitionType, State, State, Event)>;
using TransitionsCallbackVector = std::vector<TransitionsCallback>;
using TransitionsCallbackStorage =
    std::unordered_map<TransitionCallbackKey, TransitionsCallbackVector,
                       TransitionCallbackKeyHash>;

TransitionsCallbackStorage transitions_callbacks_storage{};

State process_event(State current, Event event) {
  const auto next = transition_to(current, event);
  if (next == State::MAX_COUNT) {
    return next;
  }

  TransitionCallbackKey key{};

  key.type = TransitionType::After;
  key.state = next;
  auto it = transitions_callbacks_storage.find(key);
  if (it != transitions_callbacks_storage.end()) {
    for (const auto &cb : it->second) {
      if (cb) {
        cb(TransitionType::After, current, next, event);
      }
    }
  }

  key.type = TransitionType::Before;
  key.state = next;
  it = transitions_callbacks_storage.find(key);
  if (it != transitions_callbacks_storage.end()) {
    for (const auto &cb : it->second) {
      if (cb) {
        cb(TransitionType::Before, current, next, event);
      }
    }
  }

  return next;
}

void attach_callback(TransitionType type, State state,
                     TransitionsCallback callback) {
  auto key = TransitionCallbackKey{type, state};
  auto it = transitions_callbacks_storage.find(key);
  if (it == transitions_callbacks_storage.end()) {
    TransitionsCallbackVector vector{};
    vector.push_back(callback);
    transitions_callbacks_storage.insert({key, vector});
  } else {
    it->second.push_back(callback);
  }
}

int main() {
  // Initialize transitions table with State::MAX_COUNT
  std::fill(transitions_storage.begin(), transitions_storage.end(),
            State::MAX_COUNT);

  add_transition(State::Idle, State::Active, Event::Start);
  add_transition(State::Active, State::Stopped, Event::Timeout);
  add_transition(State::Active, State::Canceled, Event::Cancel);
  add_transition(State::Stopped, State::Active, Event::Restart);

  attach_callback(
      TransitionType::Before, State::Active,
      [](TransitionType type, State current, State next, Event event) {
        std::println("Transition before state = {} from = {}",
                     static_cast<int>(next), static_cast<int>(current));
      });

  auto res = process_event(State::Idle, Event::Start);
  if (res == State::MAX_COUNT) {
    std::println("Invalid transition");
  }

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
