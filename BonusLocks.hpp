#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <thread>

#include "LockStrategy.hpp"

class Backoff {
 private:
  int min_delay;
  int max_delay;
  int limit;
  std::random_device rd;
  std::mt19937 gen;

 public:
  Backoff(int min, int max) : gen(rd()) {
    min_delay = min;
    max_delay = max;
    limit = min_delay;
  }

  void backoff() {
    std::uniform_int_distribution<int> distrib(0, limit);
    int delay = distrib(gen);
    limit = std::min(max_delay, 2 * limit);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }
};

template <std::size_t N>
class TTASLock : public LockStrategy<TTASLock<N>, N> {
 private:
  std::atomic_flag state = ATOMIC_FLAG_INIT;
  int min_delay = 10;
  int max_delay = 10000;

 public:
  void lock_impl(std::size_t _) {
    Backoff backoff(min_delay, max_delay);
    while (true) {
      while (state.test()) {
        std::this_thread::yield();
      }
      if (!state.test_and_set()) {
        return;
      } else {
        backoff.backoff();
      };
    }
  }

  void unlock_impl(std::size_t _) { state.clear(); }
};

template <std::size_t N>
class TicketLock : public LockStrategy<TicketLock<N>, N> {
 private:
  std::atomic<int> ticket{};
  std::atomic<int> turn{};
  static thread_local int number;

 public:
  void lock_impl(std::size_t _) {
    number = ticket++;
    while (turn.load() != number) {
      std::this_thread::yield();
    }
  }

  void unlock_impl(std::size_t _) { turn++; }
};

template <std::size_t N>
thread_local int TicketLock<N>::number{};

template <std::size_t N>
class AndersonLock : public LockStrategy<AndersonLock<N>, N> {
 private:
  static thread_local int my_slot;
  std::atomic<int> tail{};
  std::atomic<bool> flag[N]{};

 public:
  AndersonLock() { flag[0].store(true); }

  void lock_impl(std::size_t _) {
    int slot = tail++ % N;
    my_slot = slot;
    while (!flag[slot].load()) {
      std::this_thread::yield();
    }
  }

  void unlock_impl(std::size_t _) {
    int slot = my_slot;
    flag[slot].store(false);
    flag[(slot + 1) % N].store(true);
  }
};

template <std::size_t N>
thread_local int AndersonLock<N>::my_slot = 0;

struct alignas(64) QNode {
  std::atomic<bool> locked{false};
  std::atomic<QNode*> next{nullptr};
};

template <std::size_t N>
class CLHLock : public LockStrategy<CLHLock<N>, N> {
 private:
  std::atomic<std::shared_ptr<QNode>> tail{std::make_shared<QNode>()};
  static thread_local std::shared_ptr<QNode> my_pred;
  static thread_local std::shared_ptr<QNode> my_node;

 public:
  void lock_impl(std::size_t _) {
    my_node->locked.store(true);
    std::shared_ptr<QNode> pred = tail.exchange(my_node);
    my_pred = pred;

    while (pred->locked.load()) {
      std::this_thread::yield();
    }
  }

  void unlock_impl(std::size_t _) {
    my_node->locked.store(false);
    my_node = my_pred;
    my_pred = nullptr;
  }
};

template <std::size_t N>
thread_local std::shared_ptr<QNode> CLHLock<N>::my_pred = nullptr;

template <std::size_t N>
thread_local std::shared_ptr<QNode> CLHLock<N>::my_node = std::make_shared<QNode>();

template <std::size_t N>
class MCSLock : public LockStrategy<MCSLock<N>, N> {
 private:
  std::atomic<QNode*> tail{nullptr};
  static thread_local std::unique_ptr<QNode> my_node;

 public:
  void lock_impl(std::size_t _) {
    QNode* qnode = my_node.get();
    QNode* pred = tail.exchange(qnode);
    if (pred != nullptr) {
      qnode->locked.store(true);
      pred->next.store(qnode);
      while (qnode->locked.load()) {
        std::this_thread::yield();
      }
    }
  }

  void unlock_impl(std::size_t _) {
    QNode* qnode = my_node.get();
    if (qnode->next.load() == nullptr) {
      QNode* expected = qnode;
      if (tail.compare_exchange_strong(expected, nullptr)) return;
      while (qnode->next.load() == nullptr) {
        std::this_thread::yield();
      }
    }
    qnode->next.load()->locked.store(false);
    qnode->next.store(nullptr);
  }
};

template <std::size_t N>
thread_local std::unique_ptr<QNode> MCSLock<N>::my_node = std::make_unique<QNode>();
