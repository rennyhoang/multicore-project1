#pragma once
#include <cstddef>

template <typename Derived, std::size_t MaxThreads>
class ThreadIdLockStrategy {
   public:
    void lock(std::size_t thread_id) { static_cast<Derived*>(this)->lock_impl(thread_id); }

    void unlock(std::size_t thread_id) { static_cast<Derived*>(this)->unlock_impl(thread_id); }
};
