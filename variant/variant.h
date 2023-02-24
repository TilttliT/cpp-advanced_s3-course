#pragma once

#include "variant_table.h"

template <typename Visitor, typename... Vars>
constexpr decltype(auto) visit(Visitor&& visitor, Vars&&... vars);

template <typename... Types>
struct variant : private variant_utils::index_variant<variant_utils::TriviallyDestructible<Types...>, Types...> {
  using base = variant_utils::index_variant<variant_utils::TriviallyDestructible<Types...>, Types...>;
  using base::base;
  using base::index;
  using base::reset;

  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<variant_utils::get_type_by_index_t<0, Types...>>)
      requires(std::is_default_constructible_v<variant_utils::get_type_by_index_t<0, Types...>>)
      : base(variant_utils::variant_base_index<0>) {}
  constexpr variant() = delete;

  template <size_t I, typename... Args>
  constexpr variant(in_place_index_t<I>, Args&&... args)
      requires((I < sizeof...(Types)) &&
               (std::is_constructible_v<variant_utils::get_type_by_index_t<I, Types...>, Args...>))
      : base(variant_utils::variant_base_index<I>, std::forward<Args>(args)...) {}

  template <typename T, typename... Args>
  constexpr variant(in_place_type_t<T>, Args&&... args)
      requires((std::is_constructible_v<T, Args...>)&&(variant_utils::count_type<T, Types...>() == 1))
      : base(variant_utils::variant_base_index<variant_utils::get_index_by_types<T, Types...>()>,
             std::forward<Args>(args)...) {}

  template <typename Type, typename T = variant_utils::type_chose_t<Type, Types...>>
  constexpr variant(Type&& v) noexcept(std::is_nothrow_constructible_v<T, Type>)
      requires(variant_utils::CastConstructible<Type, Types...>)
      : base(variant_utils::variant_base_index<variant_utils::get_index_by_types<T, Types...>()>,
             std::forward<Type>(v)) {}

  constexpr variant(variant const&) requires(variant_utils::TriviallyCopyConstructible<Types...>) = default;
  constexpr variant(variant const& other) noexcept(variant_utils::NoexceptCopyConstructible<Types...>)
      requires(variant_utils::CopyConstructible<Types...> && !variant_utils::TriviallyCopyConstructible<Types...>) {
    construct_impl(other);
  }
  constexpr variant(variant const&) = delete;

  template <typename Type, typename T = variant_utils::type_chose_t<Type, Types...>>
  requires(variant_utils::CastAssignable<Type, T>) constexpr variant&
  operator=(Type&& v) noexcept(variant_utils::NoexceptCastAssignable<Type, T>) {
    constexpr size_t I = variant_utils::get_index_by_types<T, Types...>();
    if (index() == I) {
      get<I>(*this) = std::forward<Type>(v);
      return *this;
    }

    if constexpr (std::is_nothrow_constructible_v<T, Type> || !std::is_nothrow_move_constructible_v<T>) {
      emplace<T>(std::forward<Type>(v));
    } else {
      emplace<T>(T(std::forward<Type>(v)));
    }
    return *this;
  }

  constexpr variant& operator=(variant const&) requires(variant_utils::TriviallyCopyAssignable<Types...>) = default;
  constexpr variant& operator=(variant const& other) noexcept(variant_utils::NoexceptCopyAssignable<Types...>)
      requires(variant_utils::CopyAssignable<Types...> && !variant_utils::TriviallyCopyAssignable<Types...>) {
    if (other.valueless_by_exception()) {
      reset();
    } else {
      variant_utils::visit_impl<true>(
          [this, &other](const auto& other_value, auto other_index) {
            if (index() == other_index) {
              variant_utils::variant_get<other_index>(*this) = other_value;
            } else {
              using T = decltype(other_value);
              if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                this->template emplace<other_index>(other_value);
              } else {
                *this = variant(other);
              }
            }
          },
          other);
    }
    return *this;
  }
  constexpr variant& operator=(variant const&) = delete;

  constexpr variant(variant&&) requires(variant_utils::TriviallyMoveConstructible<Types...>) = default;
  constexpr variant(variant&& other) noexcept(variant_utils::NoexceptMoveConstructible<Types...>)
      requires(variant_utils::MoveConstructible<Types...> && !variant_utils::TriviallyMoveConstructible<Types...>) {
    construct_impl(std::move(other));
  }
  constexpr variant(variant&&) = delete;

  constexpr variant& operator=(variant&&) requires(variant_utils::TriviallyMoveAssignable<Types...>) = default;
  constexpr variant& operator=(variant&& other) noexcept(variant_utils::NoexceptMoveAssignable<Types...>)
      requires(variant_utils::MoveAssignable<Types...> && !variant_utils::TriviallyMoveAssignable<Types...>) {
    if (other.valueless_by_exception()) {
      reset();
    } else {
      variant_utils::visit_impl<true>(
          [this](auto&& other_value, auto other_index) {
            if (index() == other_index) {
              variant_utils::variant_get<other_index>(*this) = std::move(other_value);
            } else {
              this->template emplace<other_index>(std::move(other_value));
            }
          },
          std::move(other));
    }
    return *this;
  }
  constexpr variant& operator=(variant&&) = delete;

  constexpr bool valueless_by_exception() const noexcept {
    return base::ind == variant_npos;
  }

  template <size_t I, typename... Args, typename T = variant_utils::get_type_by_index_t<I, Types...>>
  requires((I < sizeof...(Types)) && (std::is_constructible_v<T, Args...>)) T& emplace(Args&&... args) {
    reset();
    std::construct_at(std::addressof(variant_utils::variant_get<I>(*this)), std::forward<Args>(args)...);
    base::ind = I;
    return get<I>(*this);
  }

  template <typename T, typename... Args>
  requires(std::is_constructible_v<T, Args...>) T& emplace(Args&&... args) {
    return emplace<variant_utils::get_index_by_types<T, Types...>()>(std::forward<Args>(args)...);
  }

  void swap(variant& other) noexcept(variant_utils::NoexceptSwappable<Types...>) {
    if (valueless_by_exception() && !other.valueless_by_exception()) {
      *this = std::move(other);
      other.reset();
    } else if (!valueless_by_exception() && other.valueless_by_exception()) {
      other = std::move(*this);
      reset();
    } else if (!valueless_by_exception() && !other.valueless_by_exception()) {
      if (index() == other.index()) {
        visit(
            [](auto&& lhs, auto&& rhs) {
              if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
                using std::swap;
                swap(lhs, rhs);
                return;
              }
            },
            *this, other);
      } else {
        other = std::exchange(*this, std::move(other));
      }
    }
  }

private:
  template <size_t I, typename Base>
  friend constexpr auto&& variant_utils::variant_get(Base&& v);

  template <bool indexed, typename Array, typename Tuple, typename Sequence>
  friend struct variant_utils::visit_table;

  template <size_t I, typename Var>
  friend constexpr decltype(auto) get(Var&& v);

  template <size_t I, typename... Types_>
  friend constexpr decltype(auto) get_if(variant<Types_...>* v);

  template <size_t I, typename... Types_>
  friend constexpr decltype(auto) get_if(variant<Types_...> const* v);

  template <typename Var>
  constexpr void construct_impl(Var&& other) {
    base::ind = other.index();
    if (!valueless_by_exception()) {
      variant_utils::visit_impl<true>(
          [this, &other](const auto&, auto other_index) {
            this->template construct<other_index>(std::forward<Var>(other).get_base());
          },
          std::forward<Var>(other));
    }
  }
};

struct bad_variant_access : public std::exception {
  const char* what() const noexcept override {
    return "bad_variant_access";
  }
};

template <size_t, typename>
struct variant_alternative;

template <size_t I, typename... Types>
struct variant_alternative<I, variant<Types...>> {
  using type = variant_utils::get_type_by_index_t<I, Types...>;
};

template <size_t I, typename V>
struct variant_alternative<I, const V> {
  using type = const typename variant_alternative<I, V>::type;
};

template <size_t I, typename V>
using variant_alternative_t = typename variant_alternative<I, V>::type;

template <typename T, typename... Types>
constexpr bool holds_alternative(variant<Types...> const& v) noexcept {
  return v.index() == variant_utils::get_index_by_types<T, Types...>();
}

template <size_t I, typename Var>
constexpr decltype(auto) get(Var&& v) {
  if (v.index() != I)
    throw bad_variant_access();
  return variant_utils::variant_get<I>(std::forward<Var>(v).get_base());
}

template <typename T, typename Var>
constexpr decltype(auto) get(Var&& v) {
  return get<variant_utils::get_index_by_variant_v<T, std::decay_t<Var>>, Var>(std::forward<Var>(v));
}

template <size_t I, typename... Types>
constexpr decltype(auto) get_if(variant<Types...>* v) {
  return (!v || v->index() != I) ? nullptr : std::addressof(variant_utils::variant_get<I>(v->get_base()));
}

template <size_t I, typename... Types>
constexpr decltype(auto) get_if(variant<Types...> const* v) {
  return (!v || v->index() != I) ? nullptr : std::addressof(variant_utils::variant_get<I>(v->get_base()));
}

template <typename T, typename... Types>
constexpr decltype(auto) get_if(variant<Types...>* v) {
  return get_if<variant_utils::get_index_by_types<T, Types...>(), Types...>(v);
}

template <typename T, typename... Types>
constexpr decltype(auto) get_if(variant<Types...> const* v) {
  return get_if<variant_utils::get_index_by_types<T, Types...>(), Types...>(v);
}

template <typename Visitor, typename... Vars>
constexpr decltype(auto) visit(Visitor&& visitor, Vars&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  return variant_utils::visit_impl<false>(std::forward<Visitor>(visitor), std::forward<Vars>(vars)...);
}

template <typename... Types>
constexpr bool operator==(variant<Types...> const& v, variant<Types...> const& w) {
  if (v.index() != w.index())
    return false;
  if (v.valueless_by_exception())
    return true;
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs == rhs; }), v, w);
}

template <typename... Types>
constexpr bool operator!=(variant<Types...> const& v, variant<Types...> const& w) {
  if (v.index() != w.index())
    return true;
  if (v.valueless_by_exception())
    return false;
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs != rhs; }), v, w);
}

template <typename... Types>
constexpr bool operator<(variant<Types...> const& v, variant<Types...> const& w) {
  if (w.valueless_by_exception())
    return false;
  if (v.valueless_by_exception())
    return true;
  if (v.index() != w.index())
    return v.index() < w.index();
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs < rhs; }), v, w);
}

template <typename... Types>
constexpr bool operator>(variant<Types...> const& v, variant<Types...> const& w) {
  if (v.valueless_by_exception())
    return false;
  if (w.valueless_by_exception())
    return true;
  if (v.index() != w.index())
    return v.index() > w.index();
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs > rhs; }), v, w);
}

template <typename... Types>
constexpr bool operator<=(variant<Types...> const& v, variant<Types...> const& w) {
  if (v.valueless_by_exception())
    return true;
  if (w.valueless_by_exception())
    return false;
  if (v.index() != w.index())
    return v.index() < w.index();
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs <= rhs; }), v, w);
}

template <typename... Types>
constexpr bool operator>=(variant<Types...> const& v, variant<Types...> const& w) {
  if (w.valueless_by_exception())
    return true;
  if (v.valueless_by_exception())
    return false;
  if (v.index() != w.index())
    return v.index() > w.index();
  return visit(variant_utils::compare([](auto&& lhs, auto&& rhs) constexpr { return lhs >= rhs; }), v, w);
}
