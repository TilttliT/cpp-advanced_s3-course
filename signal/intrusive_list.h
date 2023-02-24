#pragma once

#include <cstddef>
#include <iterator>

namespace intrusive {
struct default_tag;

struct list_element_base {
private:
  list_element_base *next{nullptr}, *prev{nullptr};
  template <typename T, typename Tag>
  friend struct list;

public:
  ~list_element_base();
  void unlink();
  bool is_link();
  void link(list_element_base* other);
};

template <typename Tag = default_tag>
struct list_element : public list_element_base {};

template <typename T, typename Tag = default_tag>
struct list {
private:
  list_element<Tag> root;

  template <typename V>
  struct list_iterator {
    friend struct list;

    using difference_type = std::ptrdiff_t;
    using value_type = V;
    using pointer = V*;
    using reference = V&;
    using iterator_category = std::bidirectional_iterator_tag;

    list_iterator() : ptr(nullptr) {}

    template <typename U, typename std::enable_if<std::is_same<V, const U>::value>::type* = nullptr>
    list_iterator(list_iterator<U> const& other) : ptr(other.ptr) {}

    list_iterator& operator++() {
      ptr = ptr->next;
      return (*this);
    }

    list_iterator operator++(int) {
      list_iterator res(*this);
      ++(*this);
      return res;
    }

    list_iterator& operator--() {
      ptr = ptr->prev;
      return (*this);
    }

    list_iterator operator--(int) {
      list_iterator res(*this);
      --(*this);
      return res;
    }

    friend bool operator==(list_iterator const& a, list_iterator const& b) {
      return a.ptr == b.ptr;
    }

    friend bool operator!=(list_iterator const& a, list_iterator const& b) {
      return a.ptr != b.ptr;
    }

    reference operator*() const {
      return static_cast<reference>(static_cast<list_element<Tag>&>(*ptr));
    }

    pointer operator->() const {
      return static_cast<pointer>(static_cast<list_element<Tag>*>(ptr));
    }

  private:
    list_element_base* ptr;

    list_iterator(list_element_base* o_ptr) : ptr(o_ptr) {}
  };

public:
  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;

  list() {
    root.next = root.prev = &root;
  }

  list(list const&) = delete;

  list(list&& other) : list() {
    operator=(std::move(other));
  }

  ~list() {
    clear();
  }

  list& operator=(list const&) = delete;

  list& operator=(list&& other) {
    clear();
    splice(end(), other, other.begin(), other.end());
    return *this;
  }

  bool empty() const noexcept {
    return root.next == &root;
  }

  iterator get_iterator(T& x) {
    return iterator(&x);
  }

  const_iterator get_iterator(T& x) const {
    return iterator(&x);
  }

  T& front() noexcept {
    return *(begin());
  }

  T const& front() const noexcept {
    return *(begin());
  }

  void push_front(T& val) {
    insert(begin(), val);
  }

  void pop_front() noexcept {
    erase(begin());
  }

  T& back() noexcept {
    return *(--end());
  }

  T const& back() const noexcept {
    return *(--end());
  }

  void push_back(T& val) {
    insert(end(), val);
  }

  void pop_back() noexcept {
    erase(--end());
  }

  iterator begin() noexcept {
    return iterator(root.next);
  }

  const_iterator begin() const noexcept {
    return const_iterator(root.next);
  }

  iterator end() noexcept {
    return iterator(static_cast<list_element_base*>(&root));
  }

  const_iterator end() const noexcept {
    return const_iterator(const_cast<list_element_base*>(static_cast<list_element_base const*>(&root)));
  }

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  iterator insert(const_iterator pos, T& val) {
    auto& n = static_cast<list_element<Tag>&>(val);
    if (pos.ptr != &n) {
      n.unlink();
      pos.ptr->prev->link(&n);
      n.link(pos.ptr);
    }
    return iterator(&n);
  }

  iterator erase(const_iterator pos) noexcept {
    iterator res(pos.ptr->next);
    pos.ptr->unlink();
    return res;
  }

  void splice(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept {
    if (first != last) {
      auto left = first.ptr;
      auto right = last.ptr->prev;
      left->prev->link(last.ptr);
      pos.ptr->prev->link(left);
      right->link(pos.ptr);
    }
  }
};
} // namespace intrusive
