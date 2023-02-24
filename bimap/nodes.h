#pragma once

#include <cstdint>
#include <random>

namespace tags {
struct left_tag;
struct right_tag;

template <typename Tag>
struct other_tag {
  using type = void;
};

template <>
struct other_tag<left_tag> {
  using type = right_tag;
};

template <>
struct other_tag<right_tag> {
  using type = left_tag;
};


template <typename Left, typename Right, typename Tag>
struct key {
  using type = void;
};

template <typename Left, typename Right>
struct key<Left, Right, left_tag> {
  using type = Left;
};

template <typename Left, typename Right>
struct key<Left, Right, right_tag> {
  using type = Right;
};
} // namespace tags

namespace generator {
static inline auto gen = std::mt19937(std::random_device{}());
} // namespace generator

struct base_node {
  base_node* prev();
  base_node* next();
  void update_children_links();

  // Для простоты кода сделал public
  base_node* left{nullptr};
  base_node* right{nullptr};
  base_node* parent{nullptr};
};

template <typename Tag>
struct empty_node : base_node {};

struct base_bimap_node : empty_node<tags::left_tag>,
                         empty_node<tags::right_tag> {};

template <typename Left, typename Right>
struct bimap_node : base_bimap_node {
  bimap_node() = default;

  template <typename L, typename R>
  bimap_node(L&& left, R&& right)
      : left_value(std::forward<L>(left)), right_value(std::forward<R>(right)),
        priority(generator::gen()) {}

  template <typename Tag,
            std::enable_if_t<std::is_same_v<Tag, tags::left_tag>, bool> = true>
  Left const& get_value() const {
    return left_value;
  }

  template <typename Tag,
            std::enable_if_t<std::is_same_v<Tag, tags::right_tag>, bool> = true>
  Right const& get_value() const {
    return right_value;
  }

  uint32_t get_priority() const {
    return priority;
  }

private:
  Left left_value;
  Right right_value;

  // Для разных деревьев можно делать одинаковый приоритет, это ничего не
  // испортит
  uint32_t priority;
};

namespace casts {
template <typename Left, typename Right, typename Tag>
bimap_node<Left, Right>* down_cast(base_node* n) {
  return static_cast<bimap_node<Left, Right>*>(
      static_cast<base_bimap_node*>(static_cast<empty_node<Tag>*>(n)));
}
template <typename Left, typename Right, typename Tag>
base_node* up_cast(bimap_node<Left, Right>* n) {
  return static_cast<empty_node<Tag>*>(
      static_cast<base_bimap_node*>(static_cast<bimap_node<Left, Right>*>(n)));
}
} // namespace casts