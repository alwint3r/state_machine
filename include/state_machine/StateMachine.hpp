#pragma once

#include "EnumUtils.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace state_machine {

template <typename T>
concept StateID = std::is_enum_v<T> &&
                  std::is_integral_v<std::underlying_type_t<T>> && requires {
                    { std::to_underlying(T::MAX_VALUE) } -> std::integral;
                  };

template <typename T>
concept EventID = StateID<T>;

enum class TransitionType {
  Enter,
  Exit,
};

template <StateID State, EventID Event>
using TransitionCallbackFn =
    std::function<void(TransitionType, State, State, Event)>;

template <StateID State, EventID Event>
using TransitionGuard = std::function<bool(State, State, Event)>;

enum class ProcessEventErr {
  TransitionForbidden,
  NoNextStateFound,
};

template <StateID S, EventID E> class FSM {
 public:
  FSM(S initial) : currentState_(initial) {}

  void init() {
    std::ranges::fill(transitionStorage_, S::MAX_VALUE);
    for (auto &guard : transitionGuards_) {
      guard = {};
    }
    for (auto &callbacks : transitionCallbacks_) {
      callbacks.clear();
    }
  }

  void attachOnEnterStateCallback(S state,
                                  TransitionCallbackFn<S, E> callback) {
    attachTransitionCallback(TransitionType::Enter, state, std::move(callback));
  }

  void attachOnExitStateCallback(S state, TransitionCallbackFn<S, E> callback) {
    attachTransitionCallback(TransitionType::Exit, state, std::move(callback));
  }

  void enableTransition(S from, S to, E onEvent) {
    transitionSpan_(stateIndex(from), eventIndex(onEvent)) = to;
  }

  void disableTransition(S from, S /*to*/, E onEvent) {
    transitionSpan_(stateIndex(from), eventIndex(onEvent)) = S::MAX_VALUE;
  }

  void attachTransitionGuard(S state, TransitionGuard<S, E> guard) {
    transitionGuards_[stateIndex(state)] = std::move(guard);
  }

  std::expected<S, ProcessEventErr> processEvent(E event) {
    const auto nextState = computeTransition(currentState_, event);
    if (nextState == S::MAX_VALUE) {
      return std::unexpected(ProcessEventErr::NoNextStateFound);
    }

    auto &guard = transitionGuards_[stateIndex(currentState_)];
    if (guard) {
      bool result = std::invoke(guard, currentState_, nextState, event);
      if (!result) {
        return std::unexpected(ProcessEventErr::TransitionForbidden);
      }
    }

    for (const auto &cb : transitionCallbacks_[callbackIndex(
             TransitionType::Exit, currentState_)]) {
      if (cb) {
        std::invoke(cb, TransitionType::Exit, currentState_, nextState, event);
      }
    }

    const S prevState = currentState_;
    currentState_ = nextState;

    for (const auto &cb : transitionCallbacks_[callbackIndex(
             TransitionType::Enter, nextState)]) {
      if (cb) {
        std::invoke(cb, TransitionType::Enter, prevState, nextState, event);
      }
    }

    return nextState;
  }

  S getCurrentState() const { return currentState_; }

 private:
  static constexpr size_t stateIndex(S state) noexcept {
    return static_cast<size_t>(state);
  }

  static constexpr size_t eventIndex(E event) noexcept {
    return static_cast<size_t>(event);
  }

  static constexpr size_t typeIndex(TransitionType type) noexcept {
    switch (type) {
    case TransitionType::Enter:
      return 0;
    case TransitionType::Exit:
      return 1;
    }
    return 0;
  }

  static constexpr size_t callbackIndex(TransitionType type, S state) noexcept {
    return (typeIndex(type) * StateSize) + stateIndex(state);
  }

  S computeTransition(S state, E event) {
    return transitionSpan_(stateIndex(state), eventIndex(event));
  }

  void attachTransitionCallback(TransitionType type, S state,
                                TransitionCallbackFn<S, E> callback) {
    transitionCallbacks_[callbackIndex(type, state)].push_back(
        std::move(callback));
  }

 private:
  S currentState_{};

  static constexpr auto StateSize = enum_utils::enum_size_v<S>;
  static constexpr auto EventSize = enum_utils::enum_size_v<E>;
  static constexpr auto CallbackSlotCount = 2 * StateSize;

  struct TransitionTableView {
    S *data{};

    constexpr S &operator()(size_t stateIdx, size_t eventIdx) noexcept {
      return data[(stateIdx * EventSize) + eventIdx];
    }

    constexpr const S &operator()(size_t stateIdx,
                                  size_t eventIdx) const noexcept {
      return data[(stateIdx * EventSize) + eventIdx];
    }
  };

  std::array<S, StateSize * EventSize> transitionStorage_{};
  TransitionTableView transitionSpan_{transitionStorage_.data()};

  using TransitionCallbackVector = std::vector<TransitionCallbackFn<S, E>>;
  std::array<TransitionCallbackVector, CallbackSlotCount> transitionCallbacks_{};

  std::array<TransitionGuard<S, E>, StateSize> transitionGuards_{};
};

} // namespace state_machine
