#pragma once
#include <cstddef>

template <typename LockType>
void worker(std::size_t thread_id, LockType& lock_ref, int amount, int& counter) {
    for (int i = 0; i < amount; ++i) {
        lock_ref.lock(thread_id);
        counter += 1;
        lock_ref.unlock(thread_id);
    }
}