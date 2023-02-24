#pragma once

#include "variant_utils.h"

namespace variant_utils {
template <typename... Types>
concept TriviallyDestructible = (std::is_trivially_destructible_v<Types> && ...);

template <typename... Types>
concept CopyConstructible = (std::is_copy_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyCopyConstructible = (std::is_trivially_copy_constructible_v<Types> && ...);

template <typename... Types>
concept NoexceptCopyConstructible = (std::is_nothrow_copy_constructible_v<Types> && ...);

template <typename... Types>
concept CopyAssignable = (std::is_copy_assignable_v<Types> && ...) && (std::is_copy_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyCopyAssignable =
    (((std::is_trivially_copy_assignable_v<Types>)&&(std::is_trivially_copy_constructible_v<Types>)) && ...);

template <typename... Types>
concept NoexceptCopyAssignable =
    (((std::is_nothrow_copy_assignable_v<Types>)&&(std::is_nothrow_copy_constructible_v<Types>)) && ...);

template <typename... Types>
concept MoveConstructible = (std::is_move_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyMoveConstructible = (std::is_trivially_move_constructible_v<Types> && ...);

template <typename... Types>
concept NoexceptMoveConstructible = (std::is_nothrow_move_constructible_v<Types> && ...);

template <typename... Types>
concept MoveAssignable = (std::is_move_assignable_v<Types> && ...) && (std::is_move_constructible_v<Types> && ...);

template <typename... Types>
concept TriviallyMoveAssignable =
    (((std::is_trivially_move_assignable_v<Types>)&&(std::is_trivially_move_constructible_v<Types>)) && ...);

template <typename... Types>
concept NoexceptMoveAssignable =
    (((std::is_nothrow_move_assignable_v<Types>)&&(std::is_nothrow_move_constructible_v<Types>)) && ...);

template <typename T, typename... Types>
concept CastConstructible =
    !std::is_same_v<std::decay_t<T>, variant<Types...>> && !is_in_place_index_v<std::decay_t<T>> &&
    !is_in_place_type_v<std::decay_t<T>> && (std::is_constructible_v<Types, T> || ...);

template <typename Type, typename T>
concept CastAssignable = (std::is_constructible_v<T, Type> && std::is_assignable_v<T&, Type>);

template <typename Type, typename T>
concept NoexceptCastAssignable = (std::is_nothrow_constructible_v<T, Type> && std::is_nothrow_assignable_v<T&, Type>);

template <typename... Types>
concept NoexceptSwappable = ((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_swappable_v<Types>)&&...);
} // namespace variant_utils
