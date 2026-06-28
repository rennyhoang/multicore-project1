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

## Benchmark

Requires [`hyperfine`](https://github.com/sharkdp/hyperfine) and Python 3 with `matplotlib`.

```bash
bash benchmark.sh
```

Runs all 9 algorithms at thread counts {1, 2, 4, 8, 16} with 5000 ops/thread (10 timed runs each via hyperfine) and produces:

- `bench_out/{algo}_t{n}.txt` — stdout from a verification run
- `bench_out/{algo}_t{n}.json` — hyperfine timing JSON
- `benchmark_report.md` — throughput and turnaround tables + discussion
- `throughput_comparison.png` / `turnaround_comparison.png` — plots
