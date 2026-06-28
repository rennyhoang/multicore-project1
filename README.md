# multicore-project1

All base locking algorithms (filter, ttree, bakery) were written based on psuedocode in class lecture notes.
All bonus locking algorithms were written based on textbook pseudocode/descriptions.

## Build

```bash
g++ -std=c++23 -O2 -pthread -o lockbench main.cpp
```

## Run

```
./lockbench <algo> <thread_count> <increment_amount>
```

`<algo>` is one of: `filterbb`, `filtertb`, `ttree`, `bakery`, `ttas`, `ticket`, `anderson`, `clh`, `mcs`

```bash
./lockbench bakery 4 1000
./lockbench mcs 8 5000
```

The program prints the final counter value and expected value.

```bash
Running using the [mcs] algorithm with 8 threads
Final Counter Value: 40000
Expected Value:      40000
```
