#pragma once

#include "nodes.h"

template <typename Left, typename Right, typename Tag, typename Comp>
struct tree : private Comp {
  using node_t = base_node;
  using T = typename tags::key<Left, Right, Tag>::type;

  tree(base_bimap_node* empty_root, Comp comp)
      : Comp(std::move(comp)), root(static_cast<base_node*>(
                                   static_cast<empty_node<Tag>*>(empty_root))) {}
  tree(tree&&) = default;

  void insert(node_t* new_node) {
    if (root->left == nullptr) {
      root->left = new_node;
    } else {
      auto tmp = split<false>(root->left, get_value(new_node));
      tmp.first = merge(tmp.first, new_node);
      root->left = merge(tmp.first, tmp.second);
    }
    root->update_children_links();
  }

  void erase(node_t* n) {
    if (n->parent->left == n) {
      n->parent->left = merge(n->left, n->right);
    } else {
      n->parent->right = merge(n->left, n->right);
    }
    n->parent->update_children_links();
  }

  node_t* begin() const {
    node_t* res = root;
    while (res->left) {
      res = res->left;
    }
    return res;
  }

  node_t* end() const {
    return root;
  }

  node_t* lower_bound(T const& val) const {
    auto res = bound<false>(root->left, val);
    return res ? res : root;
  }

  node_t* upper_bound(T const& val) const {
    auto res = bound<true>(root->left, val);
    return res ? res : root;
  }

  node_t* find(T const& val) const {
    node_t* res = lower_bound(val);
    if (res != root && equal(get_value(res), val)) {
      return res;
    } else {
      return nullptr;
    }
  }

  // проверка на равенство через comp
  bool equal(T const& lhs, T const& rhs) const {
    return !comp(lhs, rhs) && or_equal_comp(lhs, rhs);
  }

  void swap(tree& other) {
    base_node* tmp = other.root->left;
    other.root->left = root->left;
    root->left = tmp;
    root->update_children_links();
    other.root->update_children_links();
  }

private:
  T const& get_value(node_t* n) const {
    return casts::down_cast<Left, Right, Tag>(n)->template get_value<Tag>();
  }

  uint32_t get_priority(node_t* n) const {
    return casts::down_cast<Left, Right, Tag>(n)->get_priority();
  }

  // компаратор <=
  bool or_equal_comp(T const& lhs, T const& rhs) const {
    return !comp(rhs, lhs);
  }

  bool comp(T const& lhs, T const& rhs) const {
    return this->operator()(lhs, rhs);
  }

  // Использую булевые шаблоны, как вариант, можно было бы передавать
  // компаратор, но кажется на длину кода выходит одинаково
  template <bool OrEqualComp>
  std::pair<node_t*, node_t*> split(node_t* n, T const& val) {
    if (!n) {
      return {nullptr, nullptr};
    }

    bool fl;
    if constexpr (OrEqualComp) {
      fl = or_equal_comp(get_value(n), val);
    } else {
      fl = comp(get_value(n), val);
    }
    if (fl) {
      auto res = split<OrEqualComp>(n->right, val);
      n->right = res.first;
      n->update_children_links();
      return {n, res.second};
    } else {
      auto res = split<OrEqualComp>(n->left, val);
      n->left = res.second;
      n->update_children_links();
      return {res.first, n};
    }
  }

  node_t* merge(node_t* left, node_t* right) {
    if (!left)
      return right;
    if (!right)
      return left;

    if (get_priority(left) > get_priority(right)) {
      left->right = merge(left->right, right);
      left->update_children_links();
      return left;
    } else {
      right->left = merge(left, right->left);
      right->update_children_links();
      return right;
    }
  }

  template <bool OrEqualComp>
  node_t* bound(node_t* n, T const& val) const {
    if (!n) {
      return nullptr;
    }

    bool fl;
    if constexpr (OrEqualComp) {
      fl = or_equal_comp(get_value(n), val);
    } else {
      fl = comp(get_value(n), val);
    }
    if (fl) {
      return bound<OrEqualComp>(n->right, val);
    } else {
      node_t* res = bound<OrEqualComp>(n->left, val);
      return res ? res : n;
    }
  }

  // указатель на корень, созданный статически в bimap.h
  node_t* root;
};
