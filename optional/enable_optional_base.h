#pragma once

#include <type_traits>

template <typename T, bool enabled = std::is_copy_constructible_v<T>>
struct copy_base {
  copy_base() = default;
  copy_base(const copy_base&) = delete;
  copy_base(copy_base&&) = default;
  copy_base& operator=(const copy_base&) = default;
  copy_base& operator=(copy_base&&) = default;
};

template <typename T>
struct copy_base<T, true> {};

template <typename T, bool enabled = std::is_copy_assignable_v<T>&&
                          std::is_copy_constructible_v<T>>
struct copy_assign_base {
  copy_assign_base() = default;
  copy_assign_base(const copy_assign_base&) = default;
  copy_assign_base(copy_assign_base&&) = default;
  copy_assign_base& operator=(const copy_assign_base&) = delete;
  copy_assign_base& operator=(copy_assign_base&&) = default;
};

template <typename T>
struct copy_assign_base<T, true> {};

template <typename T, bool enabled = std::is_move_constructible_v<T>>
struct move_base {
  move_base() = default;
  move_base(const move_base&) = default;
  move_base(move_base&&) = delete;
  move_base& operator=(const move_base&) = default;
  move_base& operator=(move_base&&) = default;
};

template <typename T>
struct move_base<T, true> {};

template <typename T, bool enabled = std::is_move_assignable_v<T>&&
                          std::is_move_constructible_v<T>>
struct move_assign_base {
  move_assign_base() = default;
  move_assign_base(const move_assign_base&) = default;
  move_assign_base(move_assign_base&&) = default;
  move_assign_base& operator=(const move_assign_base&) = default;
  move_assign_base& operator=(move_assign_base&&) = delete;
};

template <typename T>
struct move_assign_base<T, true> {};
