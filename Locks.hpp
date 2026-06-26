#pragma once
#include "LockStrategy.hpp"
#include <atomic>
#include <thread>
#include <array>

class PetersonLock : public ThreadIdLockStrategy<PetersonLock, 2> {
private:
    std::atomic<bool> flag_[2]{};
    std::atomic<std::size_t> victim_{};

public:
    void lock_impl(std::size_t me) {
        std::size_t other = 1 - me;
        flag_[me].store(true);
        victim_.store(me);
        
        while (flag_[other].load() && victim_.load() == me) {
            std::this_thread::yield(); 
        }
    }

    void unlock_impl(std::size_t me) {
        flag_[me].store(false);
    }
};

template <std::size_t N>
class GPLock : public ThreadIdLockStrategy<GPLock<N>, N> {
private:
    std::atomic<bool> flag_[N]{};
    std::atomic<int> victim_{};

public:
    void lock_impl(std::size_t me) {
        flag_[me].store(true);
        victim_.store(me);
        while (true) {
            bool flags_down{true};
            for(std::size_t other = 0; other < N; other++) {
                if(other == me) continue;
                if(flag_[other].load()) flags_down = false;
            }
            if(victim_.load() == me && !flags_down) {
                std::this_thread::yield(); 
            } else {
                break;
            }
        };
    }

    void unlock_impl(std::size_t me) {
        flag_[me].store(false);
    }
};

template <std::size_t N>
class FilterBBLock : public ThreadIdLockStrategy<FilterBBLock<N>, N> {
private:
    std::array<GPLock<N>, N> Rungs{};
    
public:
    void lock_impl(std::size_t me) {
        for(std::size_t i = 0; i < N; i++) {
            Rungs[i].lock(me);
        }
    }

    void unlock_impl(std::size_t me) {
        for(int i = N - 1; i >= 0; i--) {
            Rungs[i].unlock(me);
        }
    }
};

template <std::size_t N>
class FilterTBLock : public ThreadIdLockStrategy<FilterTBLock<N>, N> {
private:
    std::atomic<int> level_[N]{};
    std::atomic<int> victim_[N]{};
public:
    void lock_impl(std::size_t me) {
        for(std::size_t l = 1; l < N; l++) {
            level_[me].store(l);
            victim_[l].store(me);
            while (true) {
                bool above_me{false};
                for(std::size_t other = 0; other < N; other++) {
                    if(other == me) continue;
                    if(level_[other].load() >= l) above_me = true;
                }
                if(victim_[l].load() == me && above_me) {
                    std::this_thread::yield(); 
                } else {
                    break;
                }
            }
        }
    }

    void unlock_impl(std::size_t me) {
        level_[me] = 0;
    }
};

template <std::size_t N>
class TournamentTreeLock : public ThreadIdLockStrategy<TournamentTreeLock<N>, N> {
private:
    std::array<PetersonLock, N-1> nodes{};
    
public:
    void acquire_while_ascending(std::size_t id, std::size_t total, std::size_t offset) {
        if (total == 1) return;
        std::size_t instance = id / 2;
        std::size_t side = id % 2;
        nodes[offset + instance].lock(side);
        acquire_while_ascending(instance, total / 2, offset + total / 2);
    }
    void release_while_descending(std::size_t id, std::size_t total, std::size_t offset) {
        if (total == 1) return;
        std::size_t instance = id / 2;
        std::size_t side = id % 2;
        release_while_descending(instance, total / 2, offset + total / 2);
        nodes[offset + instance].unlock(side);
    }
    void lock_impl(std::size_t me) {
        acquire_while_ascending(me, N, 0);
    }

    void unlock_impl(std::size_t me) {
        release_while_descending(me, N, 0);
    }
};

template <std::size_t N>
class BakeryLock : public ThreadIdLockStrategy<BakeryLock<N>, N> {
private:
    std::atomic<bool> flag_[N]{};
    std::atomic<int> token_[N]{};
public:
    void lock_impl(std::size_t me) {
        flag_[me].store(true);
        int max_token{};
        for(std::size_t i = 0; i < N; i++) {
            int curr_token = token_[i].load();
            if (curr_token > max_token) {
                max_token = curr_token;
            }
        }
        token_[me].store(max_token + 1);
        flag_[me].store(false);
        
        for(std::size_t other = 0; other < N; other++) {
            if(other == me) continue;
            while(flag_[other].load()) { std::this_thread::yield(); }
            while(true) {
                int my_token = token_[me].load();
                int other_token = token_[other].load();
                if (other_token != 0 && (my_token > other_token || (my_token == other_token && other < me))) {
                    std::this_thread::yield();
                } else {
                    break;
                }
            }
        }
    }

    void unlock_impl(std::size_t me) {
        token_[me].store(0);
    }
};