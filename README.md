# multicore-project1

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
