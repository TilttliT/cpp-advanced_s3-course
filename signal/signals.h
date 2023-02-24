#pragma once
#include "intrusive_list.h"
#include <functional>

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using slot = std::function<void(Args...)>;
  struct connection_tag;

  struct connection : intrusive::list_element<connection_tag> {
    connection() = default;

    connection(connection&& other) {
      move_connection(other);
    }

    connection& operator=(connection&& other) {
      if (this != &other) {
        disconnect();
        move_connection(other);
      }
      return *this;
    }

    void disconnect() {
      if (this->is_link())
        for (iterator_holder* p = sig->tail; p != nullptr; p = p->prev)
          if (p->current != sig->connections.end() && &*p->current == this)
            ++p->current;
      clear();
    }

    void operator()(Args... args) const {
      func(args...);
    }

    ~connection() {
      disconnect();
    }

  private:
    friend signal;

    void move_connection(connection& other) {
      func = std::move(other.func);
      sig = other.sig;
      if (other.is_link()) {
        sig->connections.insert(std::next(sig->connections.get_iterator(other)), *this);
        other.disconnect();
      }
    }

    void clear() {
      this->unlink();
      sig = nullptr;
      func = {};
    }

    connection(signal* sig_, slot func_) : sig(sig_), func(std::move(func_)) {
      sig->connections.push_back(*this);
    }

    signal* sig;
    slot func;
  };

  using connections_list = intrusive::list<connection, connection_tag>;

  signal() = default;

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  ~signal() {
    while (!connections.empty())
      connections.back().clear();
    for (iterator_holder* p = tail; p != nullptr; p = p->prev)
      p->sig = nullptr;
  }

  connection connect(std::function<void(Args...)> slot) noexcept {
    return connection(this, std::move(slot));
  }

  void operator()(Args... args) const {
    iterator_holder holder(this);
    while (holder.current != connections.end()) {
      auto copy = holder.current;
      ++holder.current;
      (*copy)(args...);
      if (holder.sig == nullptr)
        return;
    }
  }

private:
  struct iterator_holder {
    explicit iterator_holder(const signal* sig_) : sig(sig_), current(sig->connections.begin()), prev(sig->tail) {
      sig->tail = this;
    }

    ~iterator_holder() {
      if (sig != nullptr) {
        sig->tail = prev;
      }
    }

    const signal* sig;
    typename connections_list::const_iterator current;
    iterator_holder* prev;
  };

  connections_list connections;
  mutable iterator_holder* tail = nullptr;
};

} // namespace signals
