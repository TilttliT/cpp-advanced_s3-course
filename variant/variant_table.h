#pragma once

#include "variant_base.h"
#include <cstddef>
#include <tuple>

template <size_t I, typename Var>
constexpr decltype(auto) get(Var&& v);

namespace variant_utils {
template <typename T>
using visit_zeroes = std::integral_constant<size_t, 0>;

template <bool indexed, typename Visitor, typename... Vars>
struct visit_return;

template <typename Visitor, typename... Vars>
struct visit_return<false, Visitor, Vars...>
    : std::invoke_result<Visitor&&, decltype(get<0>(std::declval<Vars>()))...> {};

template <typename Visitor, typename... Vars>
struct visit_return<true, Visitor, Vars...>
    : std::invoke_result<Visitor&&, decltype(get<0>(std::declval<Vars>()))..., visit_zeroes<Vars>...> {};

template <bool indexed, typename Visitor, typename... Vars>
using visit_return_t = typename visit_return<indexed, Visitor, Vars...>::type;

template <typename T>
struct variant_types_count;

template <typename... Types>
struct variant_types_count<variant<Types...>> : variant_size<variant<Types...>> {};

template <bool trivial, typename... Types>
struct variant_types_count<index_variant<trivial, Types...>> : variant_types_count<variant<Types...>> {};

template <typename T>
inline constexpr size_t variant_types_count_v = variant_types_count<T>::value;

template <typename T, size_t... Sizes>
struct rec_array {
  constexpr T const& get() const {
    return array;
  }
  T array;
};

template <typename T, size_t S, size_t... Sizes>
struct rec_array<T, S, Sizes...> {
  template <typename... Indexes>
  constexpr T const& get(size_t i, Indexes... inds) const {
    return array[i].get(inds...);
  }
  rec_array<T, Sizes...> array[S];
};

template <bool indexed, typename Array, typename Tuple, typename Sequence>
struct visit_table;

template <bool indexed, typename R, typename Vis, size_t... Sizes, typename... Vars, size_t... Inds>
struct visit_table<indexed, rec_array<R (*)(Vis, Vars...), Sizes...>, std::tuple<Vars...>,
                   std::index_sequence<Inds...>> {
  using array_t = rec_array<R (*)(Vis&&, Vars&&...), Sizes...>;
  using var_type = std::decay_t<get_type_by_index_t<sizeof...(Inds), Vars...>>;

  template <size_t I, typename Element>
  static constexpr void make_element(Element& element) {
    element =
        visit_table<indexed, std::decay_t<Element>, std::tuple<Vars...>, std::index_sequence<Inds..., I>>::make_table();
  }

  template <size_t... Indexes>
  static constexpr void make_array(array_t& table, std::index_sequence<Indexes...>) {
    (make_element<Indexes>(table.array[Indexes]), ...);
  }

  static constexpr array_t make_table() {
    array_t visit_table{};
    make_array(visit_table, std::make_index_sequence<variant_types_count_v<var_type>>());
    return visit_table;
  }
};

template <bool indexed, typename R, typename Vis, typename... Vars, size_t... Inds>
struct visit_table<indexed, rec_array<R (*)(Vis, Vars...)>, std::tuple<Vars...>, std::index_sequence<Inds...>> {
  using array_t = rec_array<R (*)(Vis&&, Vars&&...)>;

  static constexpr decltype(auto) invoke(Vis&& vis, Vars&&... vars) {
    if constexpr (indexed) {
      return std::forward<Vis>(vis)(variant_get<Inds>(std::forward<Vars>(vars).get_base())...,
                                    std::integral_constant<size_t, Inds>{}...);
    } else {
      return std::forward<Vis>(vis)(variant_get<Inds>(std::forward<Vars>(vars).get_base())...);
    }
  }

  static constexpr array_t make_table() {
    return array_t{&invoke};
  }
};

template <bool indexed, typename R, typename Vis, typename... Vars>
struct generate_visit_table {
  static constexpr auto table =
      visit_table<indexed, rec_array<R (*)(Vis&&, Vars&&...), variant_types_count_v<std::decay_t<Vars>>...>,
                  std::tuple<Vars...>, std::index_sequence<>>::make_table();
};

template <bool indexed, typename Visitor, typename... Vars>
constexpr decltype(auto) visit_impl(Visitor&& visitor, Vars&&... vars) {
  constexpr auto& table =
      generate_visit_table<indexed, visit_return_t<indexed, Visitor, Vars...>, Visitor&&, Vars&&...>::table;
  auto f = table.get(vars.index()...);
  return (*f)(std::forward<Visitor>(visitor), std::forward<Vars>(vars)...);
}
} // namespace variant_utils
