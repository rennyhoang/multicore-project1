#pragma once
#include "LockStrategy.hpp"
#include <atomic>
#include <memory>
#include <thread>
#include <array>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <new>

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
class TTASLock : public ThreadIdLockStrategy<TTASLock<N>, N> {
private:
	std::atomic_flag state = ATOMIC_FLAG_INIT;
	int min_delay = 10;
	int max_delay = 10000;

public:
	void lock_impl(std::size_t me) {
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

	void unlock_impl(std::size_t me) {
		state.clear();
	}
};

template <std::size_t N>
class TicketLock : public ThreadIdLockStrategy<TicketLock<N>, N> {
private:
	std::atomic<int> ticket_{};
	std::atomic<int> turn_{};
	static thread_local int number;

public:
	void lock_impl(std::size_t me) {
		number = ticket_++;
		while(turn_.load() != number) { std::this_thread::yield(); }
	}

	void unlock_impl(std::size_t me) {
		turn_++;
	}
};

template <std::size_t N>
thread_local int TicketLock<N>::number{};

template <std::size_t N>
class AndersonLock : public ThreadIdLockStrategy<AndersonLock<N>, N> {
private:
	static thread_local int my_slot;
	std::atomic<int> tail_{};
	std::atomic<bool> flag_[N] {};

public:
	AndersonLock() {
		flag_[0].store(true);
	}

	void lock_impl(std::size_t me) {
		int slot = tail_++ % N;
		my_slot = slot;
		while (!flag_[slot].load()) {
			std::this_thread::yield();
		}
	}

	void unlock_impl(std::size_t me) {
		int slot = my_slot;
		flag_[slot].store(false);
		flag_[(slot + 1) % N].store(true);
	}
};

template <std::size_t N>
thread_local int AndersonLock<N>::my_slot = 0;

struct alignas(64) QNode {
	std::atomic<bool> locked{false};
	std::atomic<QNode*> next{nullptr};
};

template <std::size_t N>
class CLHLock : public ThreadIdLockStrategy<CLHLock<N>, N> {
private:
	std::atomic<std::shared_ptr<QNode>> tail_{std::make_shared<QNode>()};
	static thread_local std::shared_ptr<QNode> my_pred;
	static thread_local std::shared_ptr<QNode> my_node;

public:
	void lock_impl(std::size_t me) {
		my_node->locked.store(true);
    std::shared_ptr<QNode> pred = tail_.exchange(my_node);
		my_pred = pred;

		while(pred->locked.load()) {
			std::this_thread::yield();
		}
	}

	void unlock_impl(std::size_t me) {
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
class MCSLock : public ThreadIdLockStrategy<MCSLock<N>, N> {
private:
	std::atomic<QNode*> tail_{nullptr};
	static thread_local std::unique_ptr<QNode> my_node;

public:
	void lock_impl(std::size_t me) {
	  QNode* qnode = my_node.get();
		QNode* pred = tail_.exchange(qnode);
        if (pred != nullptr) {
            qnode->locked.store(true);
            pred->next.store(qnode);
            while(qnode->locked.load()) {
			    std::this_thread::yield();
		    } 
        }
	}

	void unlock_impl(std::size_t me) {
		QNode* qnode = my_node.get();
		if (qnode->next.load() == nullptr) {
		    QNode* expected = qnode;
		    if (tail_.compare_exchange_strong(expected, nullptr)) return;
		    while (qnode->next.load() == nullptr) { std::this_thread::yield(); }
		}
		qnode->next.load()->locked.store(false);
		qnode->next.store(nullptr);
	}
};

template <std::size_t N>
thread_local std::unique_ptr<QNode> MCSLock<N>::my_node = std::make_unique<QNode>();
