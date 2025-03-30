#!/bin/bash

# 编译程序
echo "编译程序..."
g++ -O2 -g matrix_vector.cpp -o matrix_vector
g++ -O2 -g sum_array.cpp -o sum_array

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

# 创建结果目录
mkdir -p results

# 1. 矩阵向量内积 - 基本性能测试
echo "运行矩阵向量内积性能测试..."
./matrix_vector 0 3000 500 > results/matrix_vector_results.csv

# 2. n个数求和 - 基本性能测试
echo "运行n个数求和性能测试..."
./sum_array 0 24 > results/sum_array_results.csv

# 检查是否可以使用perf
if ! command -v perf &> /dev/null; then
    echo "perf工具未安装，跳过性能分析部分。"
    echo "可以尝试安装: sudo apt install linux-tools-common linux-tools-generic linux-tools-\$(uname -r)"
    exit 0
fi

# 3. 使用perf进行性能分析

# 矩阵向量内积 - 平凡算法性能分析
echo "对矩阵向量内积平凡算法进行性能分析..."
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses \
    ./matrix_vector 1 2000 2000 \
    2> results/matrix_vector_normal_perf.txt

# 矩阵向量内积 - cache优化算法性能分析
echo "对矩阵向量内积cache优化算法进行性能分析..."
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses \
    ./matrix_vector 2 2000 2000 \
    2> results/matrix_vector_cache_perf.txt

# n个数求和 - 平凡算法性能分析
echo "对n个数求和平凡算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 1 22 \
    2> results/sum_array_normal_perf.txt

# n个数求和 - 双链路算法性能分析
echo "对n个数求和双链路算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 2 22 \
    2> results/sum_array_double_perf.txt

# n个数求和 - 递归算法性能分析
echo "对n个数求和递归算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 3 22 \
    2> results/sum_array_recursive_perf.txt

# 使用perf record收集更详细性能数据

# 矩阵向量内积 - 平凡算法
echo "记录矩阵向量内积平凡算法详细性能数据..."
perf record -e cache-misses,cycles -g ./matrix_vector 1 2000 2000
perf report --stdio > results/matrix_vector_normal_record.txt
rm -f perf.data

# 矩阵向量内积 - cache优化算法
echo "记录矩阵向量内积cache优化算法详细性能数据..."
perf record -e cache-misses,cycles -g ./matrix_vector 2 2000 2000
perf report --stdio > results/matrix_vector_cache_record.txt
rm -f perf.data

echo "测试完成！结果保存在results目录中"
ls -l results/ 