// Lightweight enum utilities tailored for enums with a MAX_COUNT sentinel.
// Requires C++20+ concepts; this project uses C++23.
//
// Usage:
//   enum class E { A, B, C, MAX_COUNT };
//   static_assert(enum_size_v<E> == 3);
//   static_assert(enum_max_v<E> == E::C);
//   constexpr auto values = enum_values<E>(); // {E::A, E::B, E::C}

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

namespace enum_utils {

// Number of valid enumerators (excluding the MAX_COUNT sentinel), assuming
// they start at 0 and are contiguous up to MAX_COUNT - 1.
template <typename E>
inline constexpr std::size_t enum_size_v = static_cast<std::size_t>(
    static_cast<std::underlying_type_t<E>>(E::MAX_VALUE));

// Largest valid enumerator (excluding MAX_COUNT sentinel).
template <typename E>
inline constexpr E enum_max_v = static_cast<E>(enum_size_v<E> - 1);

// Convenience functions if you prefer function templates over _v variables.
template <typename E> constexpr std::size_t enum_size() {
  static_assert(std::is_enum_v<E>, "E must be an enum type");
  return enum_size_v<E>;
}

template <typename E> constexpr E enum_max() {
  static_assert(std::is_enum_v<E>, "E must be an enum type");
  return enum_max_v<E>;
}

// Compile-time array of the valid enumeration values, in order, excluding
// the MAX_COUNT sentinel.
template <typename E> constexpr auto enum_values() {
  static_assert(std::is_enum_v<E>, "E must be an enum type");
  std::array<E, enum_size_v<E>> values{};
  for (std::size_t i = 0; i < enum_size_v<E>; ++i) {
    values[i] = static_cast<E>(i);
  }
  return values;
}

} // namespace enum_utils
