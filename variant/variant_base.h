#pragma once

#include "variant_concepts.h"

template <typename Visitor, typename... Vars>
constexpr decltype(auto) visit(Visitor&& visitor, Vars&&... vars);

namespace variant_utils {
template <size_t>
struct variant_base_index_t {};

template <size_t I>
inline constexpr variant_base_index_t<I> variant_base_index;

template <bool trivial, typename... Types>
struct variant_base {
  template <size_t I, typename Base>
  void construct(Base&& v) {}
};

template <typename T, typename... Types>
struct variant_base<true, T, Types...> {
  variant_base() noexcept {};

  template <size_t I, typename... Args>
  constexpr variant_base(variant_base_index_t<I>, Args&&... args)
      : tail(variant_base_index<I - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_base(variant_base_index_t<0>, Args&&... args) : head(std::forward<Args>(args)...) {}

  template <size_t I, typename Base>
  void construct(Base&& v) {
    if constexpr (I == 0)
      new (std::addressof(head)) T(std::forward<Base>(v).head);
    else
      tail.template construct<I - 1>(std::forward<Base>(v).tail);
  }

  ~variant_base() = default;

  union {
    T head;
    variant_base<true, Types...> tail;
  };
};

template <typename T, typename... Types>
struct variant_base<false, T, Types...> {
  variant_base() noexcept {};

  template <size_t I, typename... Args>
  constexpr variant_base(variant_base_index_t<I>, Args&&... args)
      : tail(variant_base_index<I - 1>, std::forward<Args>(args)...) {}

  template <typename... Args>
  constexpr variant_base(variant_base_index_t<0>, Args&&... args) : head(std::forward<Args>(args)...) {}

  template <size_t I, typename Base>
  void construct(Base&& v) {
    if constexpr (I == 0)
      new (std::addressof(head)) T(std::forward<Base>(v).head);
    else
      tail.template construct<I - 1>(std::forward<Base>(v).tail);
  }

  ~variant_base() {}

  union {
    T head;
    variant_base<false, Types...> tail;
  };
};

template <bool trivial, typename... Types>
struct index_variant : variant_base<TriviallyDestructible<Types...>, Types...> {
  using base = variant_base<TriviallyDestructible<Types...>, Types...>;

  index_variant() = default;
  index_variant(index_variant const&) = default;
  index_variant(index_variant&&) = default;
  index_variant& operator=(index_variant const&) = default;
  index_variant& operator=(index_variant&&) = default;

  template <size_t I, typename... Args>
  constexpr index_variant(variant_base_index_t<I>, Args&&... args)
      : base(variant_base_index<I>, std::forward<Args>(args)...), ind(I) {}

  constexpr base& get_base() & noexcept {
    return *this;
  }

  constexpr const base& get_base() const& noexcept {
    return *this;
  }

  constexpr base&& get_base() && noexcept {
    return std::move(*this);
  }

  constexpr const base&& get_base() const&& noexcept {
    return std::move(*this);
  }

  constexpr size_t index() const noexcept {
    return ind;
  }

  constexpr void reset() {
    ind = variant_npos;
  }

  ~index_variant() = default;

  size_t ind{variant_npos};
};

template <typename... Types>
struct index_variant<false, Types...> : index_variant<true, Types...> {
  using base = index_variant<true, Types...>;
  using base::base;

  constexpr void reset() {
    if (base::ind != variant_npos) {
      visit_impl<false>([](auto& value) { std::destroy_at(std::addressof(value)); }, *this);
      base::ind = variant_npos;
    }
  }

  ~index_variant() noexcept {
    reset();
  }
};

template <size_t I, typename Base>
constexpr auto&& variant_get(Base&& v) {
  if constexpr (I == 0) {
    return std::forward<Base>(v).head;
  } else {
    return variant_get<I - 1>(std::forward<Base>(v).tail);
  }
}
} // namespace variant_utils
