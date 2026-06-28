#pragma once
#include <cstddef>

template <typename LockType>
void worker(std::size_t me, LockType& lock, int amount, int& counter) {
  for (int i = 0; i < amount; ++i) {
    lock.lock(me);
    counter += 1;
    lock.unlock(me);
  }
}
