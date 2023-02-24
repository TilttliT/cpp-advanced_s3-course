#pragma once

#include "enable_optional_base.h"
#include "trivial_optional_base.h"

template <typename T>
struct optional : trivial_move_assign_base<T>,
                  copy_base<T>,
                  copy_assign_base<T>,
                  move_base<T>,
                  move_assign_base<T> {
  using base = trivial_move_assign_base<T>;
  using base::base;

  constexpr optional(nullopt_t) : base() {}

  optional& operator=(nullopt_t) noexcept {
    this->reset();
    return *this;
  }

  constexpr optional(T value) : optional(in_place, std::move(value)) {}

  constexpr explicit operator bool() const noexcept {
    return this->is_present;
  }

  constexpr T& operator*() noexcept {
    return this->data;
  }

  constexpr T const& operator*() const noexcept {
    return this->data;
  }

  constexpr T* operator->() noexcept {
    return &this->data;
  }

  constexpr T const* operator->() const noexcept {
    return &this->data;
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    this->reset();
    new (&this->data) T(std::forward<Args>(args)...);
    this->is_present = true;
  }
};

template <typename T>
constexpr bool operator==(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a == *b) : (!a && !b);
}

template <typename T>
constexpr bool operator!=(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a != *b) : (a || b);
}

template <typename T>
constexpr bool operator<(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a < *b) : bool(b);
}

template <typename T>
constexpr bool operator<=(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a <= *b) : (!a);
}

template <typename T>
constexpr bool operator>(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a > *b) : bool(a);
}

template <typename T>
constexpr bool operator>=(optional<T> const& a, optional<T> const& b) {
  return (a && b) ? (*a >= *b) : (!b);
}
