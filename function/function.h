#pragma once

#include <exception>

struct bad_function_call : std::exception {
  char const* what() const noexcept override {
    return "bad function call";
  }
};

namespace details {
using storage_t = std::aligned_storage_t<sizeof(void*), alignof(void*)>;

template <typename T>
constexpr inline bool fits_small =
    sizeof(T) < sizeof(storage_t) && std::is_nothrow_move_constructible_v<T> &&
    alignof(storage_t) % alignof(T) == 0 &&
    std::is_nothrow_move_assignable_v<T>;

template <typename T>
static T* get_func(storage_t* s) {
  return reinterpret_cast<T*>(s);
}

template <typename T>
static T const* get_func(storage_t const* s) {
  return reinterpret_cast<T const*>(s);
}

template <typename T>
static T* get_func_from_ptr(storage_t* s) {
  return *get_func<T*>(s);
}

template <typename T>
static T const* get_func_from_ptr(storage_t const* s) {
  return *get_func<T const*>(s);
}
} // namespace details

namespace descriptor {
template <typename R, typename... Args>
struct type_descriptor {
  void (*copy)(details::storage_t const& src, details::storage_t& dst);
  void (*move)(details::storage_t& src, details::storage_t& dst);
  void (*destroy)(details::storage_t& src);
  R (*invoke)(details::storage_t const& src, Args... args);

  template <typename T>
  static type_descriptor<R, Args...> const* get_descriptor() noexcept {
    if constexpr (details::fits_small<T>) {
      constexpr static type_descriptor<R, Args...> result = {
          [](details::storage_t const& src, details::storage_t& dst) {
            // copy
            new (&dst) T(*details::get_func<T>(&src));
          },
          [](details::storage_t& src, details::storage_t& dst) {
            // move
            new (&dst) T(std::move(*details::get_func<T>(&src)));
          },
          [](details::storage_t& src) {
            // destroy
            details::get_func<T>(&src)->~T();
          },
          [](details::storage_t const& src, Args... args) -> R {
            // invoke
            return (*details::get_func<T>(&src))(std::forward<Args>(args)...);
          }};
      return &result;
    } else {
      constexpr static type_descriptor<R, Args...> result = {
          [](details::storage_t const& src, details::storage_t& dst) {
            // copy
            auto ptr = new T(*details::get_func_from_ptr<T>(&src));
            new (&dst) T*(ptr);
          },
          [](details::storage_t& src, details::storage_t& dst) {
            // move
            new (&dst) T*(details::get_func_from_ptr<T>(&src));
            reinterpret_cast<T*&>(src) = nullptr;
          },
          [](details::storage_t& src) {
            // destroy
            delete details::get_func_from_ptr<T>(&src);
          },
          [](details::storage_t const& src, Args... args) -> R {
            // invoke
            return (*details::get_func_from_ptr<T>(&src))(
                std::forward<Args>(args)...);
          }};
      return &result;
    }
  }

  static type_descriptor<R, Args...> const* get_empty_descriptor() noexcept {
    constexpr static type_descriptor<R, Args...> result = {
        [](details::storage_t const& src, details::storage_t& dst) {
          // copy
          dst = src;
        },
        [](details::storage_t& src, details::storage_t& dst) {
          // move
          dst = src;
        },
        [](details::storage_t&) {
          // destroy
        },
        [](details::storage_t const&, Args...) -> R {
          // invoke
          throw bad_function_call{};
        }};

    return &result;
  }
};
} // namespace descriptor

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept
      : desc(descriptor::type_descriptor<R, Args...>::get_empty_descriptor()) {}

  function(function const& other) : desc(other.desc) {
    desc->copy(other.storage, this->storage);
  }

  function(function&& other) : desc(std::move(other.desc)) {
    desc->move(other.storage, this->storage);
  }

  template <typename T>
  function(T val)
      : desc(descriptor::type_descriptor<R, Args...>::template get_descriptor<
             T>()) {
    if constexpr (details::fits_small<T>) {
      new (&storage) T(std::move(val));
    } else {
      auto ptr = new T(std::move(val));
      new (&storage) T*(ptr);
    }
  }

  function& operator=(function const& rhs) {
    if (this != &rhs) {
      function(rhs).swap(*this);
    }
    return *this;
  }

  function& operator=(function&& rhs) noexcept {
    if (this != &rhs) {
      desc->destroy(storage);
      rhs.desc->move(rhs.storage, storage);
      desc = rhs.desc;
    }
    return *this;
  }

  ~function() {
    desc->destroy(storage);
  }

  explicit operator bool() const noexcept {
    return descriptor::type_descriptor<R, Args...>::get_empty_descriptor() !=
           desc;
  }

  R operator()(Args... args) const {
    return desc->invoke(storage, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    if (!check_descriptor<T>()) {
      return nullptr;
    }

    if constexpr (details::fits_small<T>) {
      return details::get_func<T>(&storage);
    } else {
      return details::get_func_from_ptr<T>(&storage);
    }
  }

  template <typename T>
  T const* target() const noexcept {
    if (!check_descriptor<T>()) {
      return nullptr;
    }

    if constexpr (details::fits_small<T>) {
      return details::get_func<T>(&storage);
    } else {
      return details::get_func_from_ptr<T>(&storage);
    }
  }

  void swap(function& other) {
    auto tmp = std::move(other);
    other = std::move(*this);
    *this = std::move(tmp);
  }

private:
  details::storage_t storage;
  descriptor::type_descriptor<R, Args...> const* desc;

  template <typename T>
  bool check_descriptor() const {
    return desc ==
           descriptor::type_descriptor<R,
                                       Args...>::template get_descriptor<T>();
  }
};
