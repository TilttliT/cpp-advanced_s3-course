#include "nodes.h"

base_node* base_node::prev() {
  base_node* n = this;
  if (n->left) {
    n = n->left;
    while (n->right) {
      n = n->right;
    }
    return n;
  }
  while (n->parent->right != n) {
    n = n->parent;
  }
  return n->parent;
}

base_node* base_node::next() {
  base_node* n = this;
  if (n->right) {
    n = n->right;
    while (n->left) {
      n = n->left;
    }
    return n;
  }
  while (n->parent->left != n) {
    n = n->parent;
  }
  return n->parent;
}

void base_node::update_children_links() {
  if (left) {
    left->parent = this;
  }
  if (right) {
    right->parent = this;
  }
}
