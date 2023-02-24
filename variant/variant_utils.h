#pragma once

#include <cstddef>
#include <exception>

template <typename... Types>
struct variant;

inline constexpr size_t variant_npos = -1;

template <typename>
struct in_place_type_t {};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type;

template <size_t>
struct in_place_index_t {};

template <size_t I>
inline constexpr in_place_index_t<I> in_place_index;

template <typename>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename V>
struct variant_size<const V> : variant_size<V> {};

template <typename T>
inline constexpr size_t variant_size_v = variant_size<T>::value;

namespace variant_utils {
template <size_t I, typename... Types>
struct get_type_by_index {
  using type = void;
};

template <size_t I, typename T, typename... Types>
struct get_type_by_index<I, T, Types...> {
  using type = typename get_type_by_index<I - 1, Types...>::type;
};

template <typename T, typename... Types>
struct get_type_by_index<0, T, Types...> {
  using type = T;
};

template <size_t I, typename... Types>
using get_type_by_index_t = typename get_type_by_index<I, Types...>::type;

template <typename V>
constexpr size_t get_index_by_types() noexcept {
  return 0;
}

template <typename V, typename T, typename... Types>
constexpr size_t get_index_by_types() noexcept {
  if constexpr (std::is_same_v<V, T>) {
    return 0;
  } else {
    return 1 + get_index_by_types<V, Types...>();
  }
}

template <typename V>
constexpr size_t count_type() noexcept {
  return 0;
}

template <typename V, typename T, typename... Types>
constexpr size_t count_type() noexcept {
  return std::is_same_v<V, T> + count_type<V, Types...>();
}

template <typename V, typename T>
concept CorrectConversion = requires(T x[], V&& t) {
  *x = {std::forward<V>(t)};
};

template <typename V, typename T, typename... Types>
struct type_chose : type_chose<V, Types...> {
  static T f(T) requires CorrectConversion<V, T>;
  using type_chose<V, Types...>::f;
};

template <typename V, typename T>
struct type_chose<V, T> {
  static T f(T) requires CorrectConversion<V, T>;
};

template <typename V, typename... Types>
using type_chose_t = decltype(type_chose<V, Types...>::f(std::declval<V>()));

decltype(auto) compare(auto&& comp) {
  return [&comp](auto&& lhs, auto&& rhs) constexpr {
    if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
      return comp(lhs, rhs);
    } else {
      return false;
    }
  };
}

template <typename T, typename Var>
struct get_index_by_variant;

template <typename T, template <typename...> typename Base, typename... Types>
struct get_index_by_variant<T, Base<Types...>> {
  static constexpr size_t value = get_index_by_types<T, Types...>();
};

template <typename T, typename Var>
inline constexpr size_t get_index_by_variant_v = get_index_by_variant<T, Var>::value;

template <typename>
struct is_in_place_type {
  static constexpr bool value = false;
};

template <typename T>
struct is_in_place_type<in_place_type_t<T>> {
  static constexpr bool value = true;
};

template <typename T>
inline constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

template <typename>
struct is_in_place_index {
  static constexpr bool value = false;
};

template <size_t I>
struct is_in_place_index<in_place_index_t<I>> {
  static constexpr bool value = true;
};

template <typename I>
inline constexpr bool is_in_place_index_v = is_in_place_index<I>::value;
} // namespace variant_utils
