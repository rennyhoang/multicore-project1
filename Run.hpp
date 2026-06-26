#pragma once
#include <thread>
#include <vector>

template <typename LockType>
void run(std::size_t num_threads, int increment_amount, int& counter) {
    LockType lock_instance;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            worker<LockType>, 
            i, 
            std::ref(lock_instance),
            increment_amount,
            std::ref(counter)
        );
    }

    for (auto& t : threads) {
        t.join();
    }
}