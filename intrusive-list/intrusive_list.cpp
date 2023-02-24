#include "intrusive_list.h"

void intrusive::list_element_base::unlink() {
  if (prev != nullptr) {
    prev->next = next;
  }
  if (next != nullptr) {
    next->prev = prev;
  }
  prev = next = nullptr;
}

void intrusive::list_element_base::link(list_element_base* other) {
  next = other;
  other->prev = this;
}

intrusive::list_element_base::~list_element_base() {
  unlink();
}
