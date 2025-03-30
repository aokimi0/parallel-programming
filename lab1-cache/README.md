# Cache Optimization Lab Report

This lab explores cache optimization techniques for matrix-vector multiplication and array summation operations.

## Project Structure

- `src/`: Source code files
  - `matrix_vector.cpp`: Matrix-vector multiplication implementations (naive and cache-optimized)
  - `sum_array.cpp`: Array summation implementations (naive, dual-path, recursive)
  - `generate_plots.py`: Script for generating performance plots
  - `plot_cache_misses.py`: Script for visualizing cache miss data
  - `run_complete_experiment.sh`: Script to run all experiments (copy in root dir)
- `bin/`: Compiled executables
  - `matrix_vector`: Matrix-vector multiplication executable
  - `sum_array`: Array summation executable
- `results/`: Contains CSV files with experiment results
- `plots/`: Generated plots from experiment data
- `cachegrind_logs/`: Cachegrind analysis files
- `cache_optimization_report.md`: Lab report summarizing findings

## Experiment Content

### 1. Matrix-Vector Multiplication Optimization

Comparing different approaches to matrix-vector multiplication:

- Column-major access (naive, cache-unfriendly)
- Row-major access (cache-friendly)
- Loop unrolling with factors 5, 10, 15, and 20

### 2. Array Summation Optimization

Comparing different approaches to array summation:

- Naive linear summation
- Dual-path summation (reducing dependency chains)
- Recursive divide-and-conquer summation

## Running the Experiments

To run all experiments and generate plots:

```bash
bash run_complete_experiment.sh
```

This script will:
1. Compile the source code into the `bin/` directory
2. Run performance tests on both algorithms
3. Collect cache miss data using Valgrind's Cachegrind tool
4. Generate performance plots in the `plots/` directory
5. Attempt to create a PDF report (requires pandoc)

### Running Individual Algorithms

Matrix-vector multiplication:

```bash
bin/matrix_vector <matrix_size> <method_id>
```

Method IDs:
- 0: Run all methods (default)
- 1: Column-major access
- 2: Row-major access
- 3-6: Loop unrolling (5, 10, 15, 20)

Array summation:

```bash
bin/sum_array <array_size>
```

## Requirements

- GCC compiler
- Python 3 with matplotlib, numpy, and pandas
- Valgrind (for cache analysis)
- Pandoc (optional, for PDF report generation)

## 实验结果

行优先的矩阵访问模式比列优先模式可以获得约10倍的性能提升，主要原因是更好地利用了CPU缓存的空间局部性。数组求和实验中，双链路算法和递归算法也展示了指令级并行和时间局部性对性能的积极影响。

详细结果可在生成的报告文件中查看：`cache_optimization_report.md` 