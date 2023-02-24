#pragma once

#include <algorithm>
#include <type_traits>

struct nullopt_t {
  constexpr explicit nullopt_t(int) {}
};
inline constexpr nullopt_t nullopt{0};

struct in_place_t {
  in_place_t() = default;
};
inline constexpr in_place_t in_place{};

template <typename T, bool trivial = std::is_trivially_destructible_v<T>>
struct trivial_destructor_base {
  constexpr trivial_destructor_base() : is_present(false), dummy(0) {}

  template <typename... Args>
  constexpr trivial_destructor_base(in_place_t, Args&&... args)
      : is_present(true), data(std::forward<Args>(args)...) {}

  void reset() {
    if (this->is_present) {
      this->data.~T();
    }
    this->is_present = false;
  }

  ~trivial_destructor_base() {
    if (is_present) {
      data.~T();
    }
  }

protected:
  bool is_present{false};
  union {
    char dummy;
    T data;
  };
};

template <typename T>
struct trivial_destructor_base<T, true> {
  constexpr trivial_destructor_base() : is_present(false), dummy(0) {}

  template <typename... Args>
  constexpr trivial_destructor_base(in_place_t, Args&&... args)
      : is_present(true), data(std::forward<Args>(args)...) {}

  void reset() {
    this->is_present = false;
  }

  ~trivial_destructor_base() = default;

  bool is_present{false};
  union {
    char dummy;
    T data;
  };
};

template <typename T, bool trivial = std::is_trivially_copy_constructible_v<T>>
struct trivial_copy_base : trivial_destructor_base<T> {
  using base = trivial_destructor_base<T>;
  using base::base;

  constexpr trivial_copy_base(const trivial_copy_base& other) : base() {
    if (other.is_present) {
      new (&this->data) T(other.data);
      this->is_present = true;
    }
  }
};

template <typename T>
struct trivial_copy_base<T, true> : trivial_destructor_base<T> {
  using base = trivial_destructor_base<T>;
  using base::base;
};

template <typename T, bool trivial = std::is_trivially_copy_assignable_v<T>&&
                          std::is_trivially_copy_constructible_v<T>>
struct trivial_copy_assign_base : trivial_copy_base<T> {
  using base = trivial_copy_base<T>;
  using base::base;

  trivial_copy_assign_base(const trivial_copy_assign_base&) = default;

  constexpr trivial_copy_assign_base&
  operator=(const trivial_copy_assign_base& other) {
    if (this != &other) {
      if (this->is_present) {
        if (other.is_present) {
          this->data = other.data;
        } else {
          this->data.~T();
        }
      } else if (other.is_present) {
        new (&this->data) T(other.data);
      }
      this->is_present = other.is_present;
    }
    return *this;
  }
};

template <typename T>
struct trivial_copy_assign_base<T, true> : trivial_copy_base<T> {
  using base = trivial_copy_base<T>;
  using base::base;
};

template <typename T, bool trivial = std::is_trivially_move_constructible_v<T>>
struct trivial_move_base : trivial_copy_assign_base<T> {
  using base = trivial_copy_assign_base<T>;
  using base::base;

  trivial_move_base(const trivial_move_base&) = default;

  constexpr trivial_move_base(trivial_move_base&& other) : base() {
    if (other.is_present) {
      new (&this->data) T(std::move(other.data));
      this->is_present = true;
    }
  }

  trivial_move_base& operator=(const trivial_move_base&) = default;
};

template <typename T>
struct trivial_move_base<T, true> : trivial_copy_assign_base<T> {
  using base = trivial_copy_assign_base<T>;
  using base::base;
};

template <typename T, bool trivial = std::is_trivially_move_assignable_v<T>&&
                          std::is_trivially_move_constructible_v<T>>
struct trivial_move_assign_base : trivial_move_base<T> {
  using base = trivial_move_base<T>;
  using base::base;

  trivial_move_assign_base(const trivial_move_assign_base&) = default;

  trivial_move_assign_base(trivial_move_assign_base&&) = default;

  trivial_move_assign_base&
  operator=(const trivial_move_assign_base&) = default;

  constexpr trivial_move_assign_base&
  operator=(trivial_move_assign_base&& other) {
    if (this != &other) {
      if (this->is_present) {
        if (other.is_present) {
          this->data = std::move(other.data);
        } else {
          this->data.~T();
        }
      } else if (other.is_present) {
        new (&this->data) T(std::move(other.data));
      }
      this->is_present = other.is_present;
    }
    return *this;
  }
};

template <typename T>
struct trivial_move_assign_base<T, true> : trivial_move_base<T> {
  using base = trivial_move_base<T>;
  using base::base;
};
