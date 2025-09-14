#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <mdspan>
#include <state_machine/EnumUtils.hpp>
#include <type_traits>
#include <unordered_map>
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

template <StateID State> struct TransitionCallbackMapKey {
  TransitionType type;
  State state;
};

struct TransitionCallbackMapKeyHash {
  template <StateID State>
  size_t operator()(const TransitionCallbackMapKey<State> &key) const noexcept {
    size_t h1 = std::hash<int>{}(std::to_underlying(key.type));
    size_t h2 = std::hash<int>{}(std::to_underlying(key.state));
    return h1 ^ (h2 << 1);
  }
};

template <StateID State>
inline bool operator==(const TransitionCallbackMapKey<State> &lhs,
                       const TransitionCallbackMapKey<State> &rhs) {
  return lhs.type == rhs.type && lhs.state == rhs.state;
}

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
  void init() { std::ranges::fill(transitionStorage_, S::MAX_VALUE); }

  void attachOnEnterStateCallback(S state,
                                  TransitionCallbackFn<S, E> callback) {
    attachTransitionCallback(TransitionType::Enter, state, callback);
  }

  void attachOnExitStateCallback(S state, TransitionCallbackFn<S, E> callback) {
    attachTransitionCallback(TransitionType::Exit, state, callback);
  }

  void enableTransition(S from, S to, E onEvent) {
    transitionSpan_[static_cast<size_t>(from), static_cast<size_t>(onEvent)] =
        to;
  }

  void disableTransition(S from, S to, E onEvent) {
    transitionSpan_[static_cast<size_t>(from), static_cast<size_t>(onEvent)] =
        S::MAX_VALUE;
  }

  void attachTransitionGuard(S state, TransitionGuard<S, E> guard) {
    transitionGuardStorage_.insert_or_assign(state, guard);
  }

  std::expected<S, ProcessEventErr> processEvent(E event) {
    const auto nextState = computeTransition(currentState_, event);
    if (nextState == S::MAX_VALUE) {
      return std::unexpected(ProcessEventErr::NoNextStateFound);
    }

    auto guardIt = transitionGuardStorage_.find(currentState_);
    if (guardIt != transitionGuardStorage_.end()) {
      if (guardIt->second) {
        bool result =
            std::invoke(guardIt->second, currentState_, nextState, event);
        if (!result) {
          return std::unexpected(ProcessEventErr::TransitionForbidden);
        }
      }
    }

    TransitionCallbackMapKey<S> key{};
    typename TransitionCallbackStorage::iterator it;

    key.type = TransitionType::Exit;
    key.state = currentState_;
    it = transitionCallbackStorage_.find(key);
    if (it != transitionCallbackStorage_.end()) {
      for (const auto &cb : it->second) {
        if (cb) {
          std::invoke(cb, TransitionType::Exit, currentState_, nextState,
                      event);
        }
      }
    }

    const S prevState = currentState_;
    currentState_ = nextState;

    key.type = TransitionType::Enter;
    key.state = nextState;
    it = transitionCallbackStorage_.find(key);
    if (it != transitionCallbackStorage_.end()) {
      for (const auto &cb : it->second) {
        if (cb) {
          std::invoke(cb, TransitionType::Enter, prevState, nextState, event);
        }
      }
    }

    return nextState;
  }

  S getCurrentState() { return currentState_; }

private:
  S computeTransition(S state, E event) {
    return transitionSpan_[static_cast<size_t>(state),
                           static_cast<size_t>(event)];
  }

  void attachTransitionCallback(TransitionType type, S state,
                                TransitionCallbackFn<S, E> callback) {
    auto key = TransitionCallbackMapKey<S>{type, state};
    auto it = transitionCallbackStorage_.find(key);
    if (it == transitionCallbackStorage_.end()) {
      TransitionCallbackVector vector{std::move(callback)};
      transitionCallbackStorage_.insert({key, vector});
    } else {
      it->second.push_back(std::move(callback));
    }
  }

private:
  S currentState_{};

  static constexpr auto StateSize = enum_utils::enum_size_v<S>;
  static constexpr auto EventSize = enum_utils::enum_size_v<E>;
  std::array<S, StateSize * EventSize> transitionStorage_{};
  std::mdspan<S, std::extents<size_t, StateSize, EventSize>> transitionSpan_{
      transitionStorage_.data()};

  using TransitionCallbackVector = std::vector<TransitionCallbackFn<S, E>>;
  using TransitionCallbackStorage =
      std::unordered_map<TransitionCallbackMapKey<S>, TransitionCallbackVector,
                         TransitionCallbackMapKeyHash>;
  TransitionCallbackStorage transitionCallbackStorage_{};

  using TransitionGuardVector = std::vector<TransitionGuard<S, E>>;
  using TransitionGuardStorage = std::unordered_map<S, TransitionGuard<S, E>>;
  TransitionGuardStorage transitionGuardStorage_{};
};

} // namespace state_machine
