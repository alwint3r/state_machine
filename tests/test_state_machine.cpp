// Basic tests for state_machine::FSM using only the C++ standard library.
// No third-party testing framework is used.

// Intentionally include headers used (directly or indirectly) by
// StateMachine.hpp to work around any missing transitive includes while keeping
// StateMachine.hpp unchanged.
#include <expected>
#include <print>
#include <vector>

#include <state_machine/StateMachine.hpp>

namespace sm = state_machine;

// Test enums for states and events
enum class TState {
  Idle,
  Active,
  Stopped,
  Canceled,
  MAX_VALUE,
};

enum class TEvent {
  Start,
  Timeout,
  Cancel,
  Restart,
  MAX_VALUE,
};

struct TestSuite {
  int passed = 0;
  int failed = 0;

  void expect_true(bool cond, std::string_view msg) {
    if (cond) {
      ++passed;
      std::println("[PASS] {}", msg);
    } else {
      ++failed;
      std::println("[FAIL] {}", msg);
    }
  }

  template <typename T, typename U>
  void expect_eq(const T &lhs, const U &rhs, std::string_view msg) {
    if (lhs == rhs) {
      ++passed;
      std::println("[PASS] {}", msg);
    } else {
      ++failed;
      std::println("[FAIL] {}", msg);
    }
  }
};

struct CallbackRecord {
  sm::TransitionType type;
  TState current;
  TState next;
  TEvent event;
};

int main() {
  TestSuite ts{};

  // Test 1: Initial state is set correctly
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    ts.expect_eq(fsm.getCurrentState(), TState::Idle, "Initial state is Idle");
  }

  // Test 2: No transition defined -> NoNextStateFound
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    auto r = fsm.processEvent(TEvent::Start);
    ts.expect_true(!r.has_value(), "Missing transition yields error");
    if (!r.has_value()) {
      ts.expect_eq(r.error(), sm::ProcessEventErr::NoNextStateFound,
                   "Error is NoNextStateFound");
    }
    ts.expect_eq(fsm.getCurrentState(), TState::Idle,
                 "State unchanged on error");
  }

  // Test 3: Simple transition Idle -> Active on Start
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    auto r = fsm.processEvent(TEvent::Start);
    ts.expect_true(r.has_value(), "Defined transition succeeds");
    if (r.has_value()) {
      ts.expect_eq(*r, TState::Active, "Next state is Active");
    }
    ts.expect_eq(fsm.getCurrentState(), TState::Active,
                 "FSM current state updated");
  }

  // Test 4: Guard prevents transition
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.attachTransitionGuard(TState::Idle,
                              [](TState cur, TState nxt, TEvent ev) {
                                (void)cur;
                                (void)nxt;
                                (void)ev;     // silence warnings
                                return false; // forbid transition
                              });
    auto r = fsm.processEvent(TEvent::Start);
    ts.expect_true(!r.has_value(), "Guarded transition fails with error");
    if (!r.has_value()) {
      ts.expect_eq(r.error(), sm::ProcessEventErr::TransitionForbidden,
                   "Error is TransitionForbidden");
    }
    ts.expect_eq(fsm.getCurrentState(), TState::Idle,
                 "State unchanged when guard blocks");
  }

  // Test 5: Exit and Enter callbacks order and arguments
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);

    std::vector<CallbackRecord> log;
    fsm.attachOnExitStateCallback(
        TState::Idle, [&](sm::TransitionType type, TState cur, TState nxt,
                          TEvent ev) { log.push_back({type, cur, nxt, ev}); });
    fsm.attachOnEnterStateCallback(
        TState::Active,
        [&](sm::TransitionType type, TState cur, TState nxt, TEvent ev) {
          log.push_back({type, cur, nxt, ev});
        });

    auto r = fsm.processEvent(TEvent::Start);
    (void)r;

    ts.expect_eq(log.size(), std::size_t{2},
                 "Two callbacks fired (exit, enter)");
    if (log.size() == 2) {
      ts.expect_eq(log[0].type, sm::TransitionType::Exit,
                   "First is Exit callback");
      ts.expect_eq(log[0].current, TState::Idle, "Exit: current is Idle");
      ts.expect_eq(log[0].next, TState::Active, "Exit: next is Active");
      ts.expect_eq(log[0].event, TEvent::Start, "Exit: event is Start");

      ts.expect_eq(log[1].type, sm::TransitionType::Enter,
                   "Second is Enter callback");
      // Expected semantics: enter callback receives previous then new state
      ts.expect_eq(log[1].current, TState::Idle,
                   "Enter: current (prev) is Idle");
      ts.expect_eq(log[1].next, TState::Active, "Enter: next is Active");
      ts.expect_eq(log[1].event, TEvent::Start, "Enter: event is Start");
    }
  }

  // Test 6: Disable transition makes it unavailable
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.disableTransition(TState::Idle, TState::Active, TEvent::Start);
    auto r = fsm.processEvent(TEvent::Start);
    ts.expect_true(!r.has_value(), "Disabled transition yields error");
    if (!r.has_value()) {
      ts.expect_eq(r.error(), sm::ProcessEventErr::NoNextStateFound,
                   "Error is NoNextStateFound after disabling");
    }
    ts.expect_eq(fsm.getCurrentState(), TState::Idle,
                 "State unchanged after disabling");
  }

  // Test 7: Chain transitions across multiple states
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.enableTransition(TState::Active, TState::Stopped, TEvent::Timeout);

    auto r1 = fsm.processEvent(TEvent::Start);
    ts.expect_true(r1.has_value(), "Idle->Active on Start works");
    ts.expect_eq(fsm.getCurrentState(), TState::Active, "State is Active");

    auto r2 = fsm.processEvent(TEvent::Timeout);
    ts.expect_true(r2.has_value(), "Active->Stopped on Timeout works");
    ts.expect_eq(fsm.getCurrentState(), TState::Stopped, "State is Stopped");
  }

  // Test 8: Multiple events from same state to same target (Idle->Active on
  // Start and Restart)
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Restart);

    auto r1 = fsm.processEvent(TEvent::Start);
    ts.expect_true(r1.has_value(), "Idle->Active on Start works (multi-event)");
    ts.expect_eq(fsm.getCurrentState(), TState::Active,
                 "State is Active after Start");

    // New instance to test the other event from Idle
    sm::FSM<TState, TEvent> fsm2(TState::Idle);
    fsm2.init();
    fsm2.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm2.enableTransition(TState::Idle, TState::Active, TEvent::Restart);
    auto r2 = fsm2.processEvent(TEvent::Restart);
    ts.expect_true(r2.has_value(),
                   "Idle->Active on Restart works (multi-event)");
    ts.expect_eq(fsm2.getCurrentState(), TState::Active,
                 "State is Active after Restart");
  }

  // Test 9: Edge - one event defined, another missing
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);

    auto ok = fsm.processEvent(TEvent::Start);
    ts.expect_true(ok.has_value(), "Defined event transitions (Start)");

    // Reset and try missing event
    sm::FSM<TState, TEvent> fsm3(TState::Idle);
    fsm3.init();
    fsm3.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    auto missing = fsm3.processEvent(TEvent::Timeout);
    ts.expect_true(!missing.has_value(),
                   "Missing event yields NoNextStateFound");
    if (!missing.has_value()) {
      ts.expect_eq(missing.error(), sm::ProcessEventErr::NoNextStateFound,
                   "Error is NoNextStateFound for missing event");
    }
  }

  // Test 10: Edge - disabling one event does not affect other event
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Restart);

    fsm.disableTransition(TState::Idle, TState::Active, TEvent::Start);

    auto s = fsm.processEvent(TEvent::Start);
    ts.expect_true(!s.has_value(), "Disabled Start no longer transitions");
    if (!s.has_value()) {
      ts.expect_eq(s.error(), sm::ProcessEventErr::NoNextStateFound,
                   "Start disabled -> NoNextStateFound");
    }

    auto r = fsm.processEvent(TEvent::Restart);
    ts.expect_true(r.has_value(), "Restart still transitions to Active");
    ts.expect_eq(fsm.getCurrentState(), TState::Active,
                 "State is Active after Restart despite Start disabled");
  }

  // Test 11: Edge - guard on state applies to all events from that state
  {
    sm::FSM<TState, TEvent> fsm(TState::Idle);
    fsm.init();
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Start);
    fsm.enableTransition(TState::Idle, TState::Active, TEvent::Restart);
    fsm.attachTransitionGuard(TState::Idle,
                              [](TState, TState, TEvent) { return false; });

    auto s = fsm.processEvent(TEvent::Start);
    ts.expect_true(!s.has_value(), "Guard blocks Start transition");
    auto r = fsm.processEvent(TEvent::Restart);
    ts.expect_true(!r.has_value(), "Guard blocks Restart transition");
  }
  std::println("\nSummary: {} passed, {} failed", ts.passed, ts.failed);
  return ts.failed == 0 ? 0 : 1;
}
