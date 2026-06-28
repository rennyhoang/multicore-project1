#pragma once
#include <cstddef>

template <typename Derived, std::size_t N>
class LockStrategy {
 public:
  void lock(std::size_t me) { static_cast<Derived*>(this)->lock_impl(me); }

  void unlock(std::size_t me) { static_cast<Derived*>(this)->unlock_impl(me); }
};
