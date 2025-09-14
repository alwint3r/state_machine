#include <state_machine/EnumUtils.hpp>
#include <state_machine/StateMachine.hpp>

#include <print>

// Concrete example of using the header-only FSM.

enum class State {
  Idle,
  Active,
  Stopped,
  Canceled,
  MAX_VALUE,
};

enum class Event {
  Start,
  Timeout,
  Cancel,
  Restart,
  MAX_VALUE,
};

namespace sm = state_machine;

int main() {
  sm::FSM<State, Event> fsm(State::Idle);
  fsm.init();

  fsm.enableTransition(State::Idle, State::Active, Event::Start);
  fsm.enableTransition(State::Active, State::Stopped, Event::Timeout);
  fsm.enableTransition(State::Active, State::Canceled, Event::Cancel);
  fsm.enableTransition(State::Stopped, State::Active, Event::Restart);

  fsm.attachOnExitStateCallback(
      State::Idle, [&](sm::TransitionType, State cur, State nxt, Event ev) {
        std::println("Exit: {} -> {} on {}", std::to_underlying(cur),
                     std::to_underlying(nxt), std::to_underlying(ev));
      });

  fsm.attachOnEnterStateCallback(
      State::Active, [&](sm::TransitionType, State prev, State nxt, Event ev) {
        std::println("Enter: {} -> {} on {}", std::to_underlying(prev),
                     std::to_underlying(nxt), std::to_underlying(ev));
      });

  fsm.attachTransitionGuard(State::Idle, [&](State cur, State nxt, Event ev) {
    std::println("Guard: {} -> {} on {}", std::to_underlying(cur),
                 std::to_underlying(nxt), std::to_underlying(ev));
    return true; // allow
  });

  auto r1 = fsm.processEvent(Event::Start);
  if (r1)
    std::println("Now in state {}", std::to_underlying(*r1));

  auto r2 = fsm.processEvent(Event::Timeout);
  if (r2)
    std::println("Now in state {}", std::to_underlying(*r2));

  auto r3 = fsm.processEvent(Event::Restart);
  if (r3)
    std::println("Now in state {}", std::to_underlying(*r3));

  return 0;
}
