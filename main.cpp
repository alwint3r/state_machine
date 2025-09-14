#include "EnumUtils.hpp"
#include <algorithm> // For std::fill
#include <array>     // For std::array
#include <expected>
#include <functional>
#include <mdspan> // For std::mdspan
#include <print>
#include <unordered_map>
#include <utility>
#include <vector>

enum class State {
  Idle,
  Active,
  Stopped,
  Canceled,
  MAX_VALUE,
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
  MAX_VALUE,
};

static constexpr auto StateSize = enum_utils::enum_size_v<State>;
static constexpr auto EventSize = enum_utils::enum_size_v<Event>;

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
               std::to_underlying(current), std::to_underlying(event),
               std::to_underlying(result), std::to_underlying(expected),
               (result == expected) ? "PASS" : "FAIL");
}

enum class TransitionType {
  Enter,
  Exit,
};

struct TransitionCallbackKey {
  TransitionType type;
  State state;
};

struct TransitionCallbackKeyHash {
  size_t operator()(const TransitionCallbackKey &key) const noexcept {
    size_t h1 = std::hash<int>{}(std::to_underlying(key.type));
    size_t h2 = std::hash<int>{}(std::to_underlying(key.state));
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

using TransitionGuard = std::function<bool(State, State, Event)>;
using TransitionGuardStorage = std::unordered_map<State, TransitionGuard>;

TransitionGuardStorage transitions_guards{};

enum class ProcessEventErr {
  TransitionForbidden,
  NoNextStateFound,
};

std::expected<State, ProcessEventErr> process_event_v2(State current,
                                                       Event event) {
  const auto next = transition_to(current, event);
  if (next == State::MAX_VALUE) {
    return std::unexpected(ProcessEventErr::NoNextStateFound);
  }

  auto transition_guard_it = transitions_guards.find(current);
  if (transition_guard_it != transitions_guards.end()) {
    if (transition_guard_it->second) {
      bool result =
          std::invoke(transition_guard_it->second, current, next, event);
      if (result == false) {
        return std::unexpected(ProcessEventErr::TransitionForbidden);
      }
    }
  }

  TransitionCallbackKey key{};
  TransitionsCallbackStorage::iterator it;

  key.type = TransitionType::Exit;
  key.state = current;
  it = transitions_callbacks_storage.find(key);
  if (it != transitions_callbacks_storage.end()) {
    for (const auto &cb : it->second) {
      if (cb) {
        cb(TransitionType::Exit, current, next, event);
      }
    }
  }

  key.type = TransitionType::Enter;
  key.state = next;
  it = transitions_callbacks_storage.find(key);
  if (it != transitions_callbacks_storage.end()) {
    for (const auto &cb : it->second) {
      if (cb) {
        cb(TransitionType::Enter, current, next, event);
      }
    }
  }
  return next;
}

void attach_transition_callback(TransitionType type, State state,
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

void attach_on_enter_callback(State state, TransitionsCallback callback) {
  attach_transition_callback(TransitionType::Enter, state, callback);
}

void attach_on_exit_callback(State state, TransitionsCallback callback) {
  attach_transition_callback(TransitionType::Exit, state, callback);
}

void attach_transition_guard(State state, TransitionGuard guard) {
  transitions_guards.insert_or_assign(state, guard);
}

int main() {
  // Demonstrate computing max enumerator programmatically via helper.
  static_assert(enum_utils::enum_max_v<State> == State::Canceled);
  static_assert(enum_utils::enum_max_v<Event> == Event::Restart);

  // Initialize transitions table with State::MAX_VALUE
  std::fill(transitions_storage.begin(), transitions_storage.end(),
            State::MAX_VALUE);

  add_transition(State::Idle, State::Active, Event::Start);
  add_transition(State::Active, State::Stopped, Event::Timeout);
  add_transition(State::Active, State::Canceled, Event::Cancel);
  add_transition(State::Stopped, State::Active, Event::Restart);

  attach_on_enter_callback(
      State::Active,
      [&](TransitionType type, State current, State next, Event event) {
        std::println("Entering state = {} from = {} by event = {}",
                     std::to_underlying(next), std::to_underlying(current),
                     std::to_underlying(event));
      });

  attach_on_exit_callback(State::Idle, [&](TransitionType type, State current,
                                           State next, Event event) {
    std::println("Exiting state = {} into = {} by event = {}",
                 std::to_underlying(current), std::to_underlying(next),
                 std::to_underlying(event));
  });

  attach_transition_guard(
      State::Idle, [&](State current, State next, Event event) {
        std::println("Guard called before transitioning into state = {} from "
                     "state = {} on event = {}",
                     std::to_underlying(next), std::to_underlying(current),
                     std::to_underlying(event));

        return true;
      });

  auto res2 = process_event_v2(State::Idle, Event::Start);
  if (res2) {
    std::println("Event processing using process_event_v2 is processed "
                 "successfully. Next State = {}",
                 std::to_underlying(*res2));
  }

  // Test all defined transitions
  test_transition(State::Idle, Event::Start, State::Active);
  test_transition(State::Active, Event::Timeout, State::Stopped);
  test_transition(State::Active, Event::Cancel, State::Canceled);
  test_transition(State::Stopped, Event::Restart, State::Active);

  // Test some invalid transitions
  test_transition(State::Idle, Event::Timeout, State::MAX_VALUE);
  test_transition(State::Canceled, Event::Restart, State::MAX_VALUE);

  return 0;
}
