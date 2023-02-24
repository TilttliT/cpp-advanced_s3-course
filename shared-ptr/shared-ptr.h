#pragma once

#include <cstddef>
#include <memory>
#include <utility>

namespace control_block {
class control_block {
private:
  size_t strong_ref = 1;
  size_t weak_ref = 0;

protected:
  virtual ~control_block() = default;
  virtual void destroy() = 0;

public:
  size_t use_count();

  void strong_inc();
  void strong_dec();
  void weak_inc();
  void weak_dec();
};

template <typename T, typename D>
class ptr_block : public control_block, private D {
private:
  T* ptr;

protected:
  void destroy() override {
    if (ptr) {
      D::operator()(ptr);
    }
  }

public:
  ptr_block(T* o_ptr, D deleter) : ptr(o_ptr), D(std::move(deleter)) {}
};

template <typename T>
class obj_block : public control_block {
public:
  template <typename... Args>
  obj_block(Args&&... args) {
    new (&obj) T(std::forward<Args...>(args)...);
  }

  void destroy() override {
    get_obj()->~T();
  }

  T* get_obj() {
    return reinterpret_cast<T*>(&obj);
  }

private:
  std::aligned_storage_t<sizeof(T), alignof(T)> obj;
};
} // namespace control_block

template <typename T>
class shared_ptr {
public:
  shared_ptr() noexcept : cb(nullptr), ptr(nullptr) {}

  shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}

  template <typename V, typename D = std::default_delete<V>>
  shared_ptr(V* o_ptr, D&& deleter = std::default_delete<V>())
      : ptr(static_cast<T*>(o_ptr)) {
    try {
      cb = new control_block::ptr_block<V, D>(o_ptr, std::forward<D>(deleter));
    } catch (...) {
      deleter(o_ptr);
      cb = nullptr;
      ptr = nullptr;
      throw;
    }
  }

  template <typename V>
  shared_ptr(const shared_ptr<V>& other) noexcept
      : cb(other.cb), ptr(static_cast<T*>(other.ptr)) {
    inc();
  }

  shared_ptr(const shared_ptr& other) noexcept : cb(other.cb), ptr(other.ptr) {
    inc();
  }

  shared_ptr(shared_ptr&& other) noexcept : cb(other.cb), ptr(other.ptr) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  template <typename V>
  shared_ptr(const shared_ptr<V>& other, T* o_ptr) noexcept
      : cb(other.cb), ptr(o_ptr) {
    inc();
  }

  ~shared_ptr() {
    dec();
  }

  template <typename V>
  shared_ptr& operator=(const shared_ptr<V>& other) noexcept {
    shared_ptr new_ptr(other);
    swap(new_ptr);
    return *this;
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (other != *this) {
      shared_ptr new_ptr(other);
      swap(new_ptr);
    }
    return *this;
  }

  template <typename V>
  shared_ptr& operator=(shared_ptr<V>&& other) noexcept {
    shared_ptr new_ptr(std::move(other));
    swap(new_ptr);
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    if (other != *this) {
      shared_ptr new_ptr(std::move(other));
      swap(new_ptr);
    }
    return *this;
  }

  T* get() const noexcept {
    return ptr;
  }

  operator bool() const noexcept {
    return ptr;
  }

  T& operator*() const noexcept {
    return *ptr;
  }

  T* operator->() const noexcept {
    return ptr;
  }

  std::size_t use_count() const noexcept {
    if (cb) {
      return cb->use_count();
    } else {
      return 0;
    }
  }

  void reset() noexcept {
    operator=(nullptr);
  }

  template <typename V, typename D = std::default_delete<V>>
  void reset(V* new_ptr, D&& deleter = std::default_delete<V>()) {
    *this = shared_ptr(new_ptr, std::forward<D>(deleter));
  }

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) {
    return lhs.get() == rhs.get();
  }
  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) {
    return lhs.get() != rhs.get();
  }

private:
  control_block::control_block* cb;
  T* ptr;

  template <typename V>
  friend class shared_ptr;
  template <typename V>
  friend class weak_ptr;
  template <typename V, typename... Args>
  friend shared_ptr<V> make_shared(Args&&...);

  void inc() {
    if (cb) {
      cb->strong_inc();
    }
  }

  void dec() {
    if (cb) {
      cb->strong_dec();
    }
  }

  shared_ptr(control_block::control_block* o_cb, T* o_ptr)
      : cb(o_cb), ptr(o_ptr) {}

  template <typename V>
  void swap(shared_ptr<V>& other) {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }
};

template <typename T>
class weak_ptr {
public:
  weak_ptr() noexcept : cb(nullptr), ptr(nullptr) {}

  weak_ptr(const weak_ptr<T>& other) noexcept : cb(other.cb), ptr(other.ptr) {
    inc();
  }

  weak_ptr(weak_ptr<T>&& other) noexcept : cb(other.cb), ptr(other.ptr) {
    other.cb = nullptr;
    other.ptr = nullptr;
  }

  weak_ptr(const shared_ptr<T>& other) noexcept : cb(other.cb), ptr(other.ptr) {
    inc();
  }

  ~weak_ptr() {
    dec();
  }

  weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
    weak_ptr new_ptr(other);
    swap(new_ptr);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr<T>& other) noexcept {
    weak_ptr new_ptr(other);
    swap(new_ptr);
    return *this;
  }

  weak_ptr& operator=(weak_ptr<T>&& other) noexcept {
    weak_ptr new_ptr(std::move(other));
    swap(new_ptr);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    if (!cb || cb->use_count() == 0) {
      return shared_ptr<T>();
    }
    cb->strong_inc();
    return shared_ptr<T>(cb, ptr);
  }

  void swap(weak_ptr& other) {
    std::swap(cb, other.cb);
    std::swap(ptr, other.ptr);
  }

private:
  control_block::control_block* cb;
  T* ptr;

  void inc() {
    if (cb) {
      cb->weak_inc();
    }
  }

  void dec() {
    if (cb) {
      cb->weak_dec();
    }
  }
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* cb = new control_block::obj_block<T>(std::forward<Args...>(args)...);
  return shared_ptr<T>(static_cast<control_block::control_block*>(cb),
                       cb->get_obj());
}
