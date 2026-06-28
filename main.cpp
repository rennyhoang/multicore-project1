#include <iostream>
#include <string>

#include "BonusLocks.hpp"
#include "Locks.hpp"
#include "Run.hpp"

constexpr std::size_t MAX_SUPPORTED_THREADS = 64;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <algo: filterbb|filtertb|ttree|bakery|ttas|ticket|anderson|clh|mcs> "
                 "<thread_count> <increment_amount>\n";
    std::cerr << "Example: " << argv[0] << " bakery 4 1000\n";
    return 1;
  }
  std::string algorithm = argv[1];
  std::size_t num_threads = 0;
  int increment_amount = 0;
  try {
    num_threads = std::stoul(argv[2]);
    increment_amount = std::stoi(argv[3]);
  } catch (const std::exception& e) {
    std::cerr << "Error: Invalid argument format for counts.\n";
    return 1;
  }
  if (num_threads == 0 || num_threads > MAX_SUPPORTED_THREADS) {
    std::cerr << "Error: Thread count must be between 1 and " << MAX_SUPPORTED_THREADS << ".\n";
    return 1;
  }

  int shared_counter = 0;

  std::cout << "Running using the [" << algorithm << "] algorithm with " << num_threads
            << " threads\n";

  if (algorithm == "bakery") {
    run<BakeryLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "filterbb") {
    run<FilterBBLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "filtertb") {
    run<FilterTBLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "ttree") {
    run<TournamentTreeLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "ttas") {
    run<TTASLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "ticket") {
    run<TicketLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "anderson") {
    run<AndersonLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "clh") {
    run<CLHLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else if (algorithm == "mcs") {
    run<MCSLock<MAX_SUPPORTED_THREADS>>(num_threads, increment_amount, shared_counter);
  } else {
    std::cerr << "Error: Unknown algorithm\n";
    return 1;
  }

  int expected_value = num_threads * increment_amount;
  std::cout << "Final Counter Value: " << shared_counter << "\n";
  std::cout << "Expected Value:      " << expected_value << "\n";

  return 0;
}
