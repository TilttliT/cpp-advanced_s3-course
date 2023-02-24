#include "shared-ptr.h"

size_t control_block::control_block::use_count() {
  return strong_ref;
}

void control_block::control_block::strong_inc() {
  ++strong_ref;
}

void control_block::control_block::strong_dec() {
  --strong_ref;
  if (strong_ref == 0) {
    destroy();
  }
  if (strong_ref + weak_ref == 0) {
    delete this;
  }
}

void control_block::control_block::weak_inc() {
  ++weak_ref;
}

void control_block::control_block::weak_dec() {
  --weak_ref;
  if (strong_ref + weak_ref == 0) {
    delete this;
  }
}
