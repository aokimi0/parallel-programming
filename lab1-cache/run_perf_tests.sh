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
mkdir -p perf_results

# 1. 使用perf测试矩阵向量内积 - 平凡算法
echo "对矩阵向量内积平凡算法进行性能分析..."
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses \
    ./matrix_vector 1 2000 2000 \
    2> perf_results/matrix_vector_normal_perf.txt

# 2. 使用perf测试矩阵向量内积 - cache优化算法
echo "对矩阵向量内积cache优化算法进行性能分析..."
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses \
    ./matrix_vector 2 2000 2000 \
    2> perf_results/matrix_vector_cache_perf.txt

# 3. 使用perf测试n个数求和 - 平凡算法
echo "对n个数求和平凡算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 1 22 \
    2> perf_results/sum_array_normal_perf.txt

# 4. 使用perf测试n个数求和 - 双链路算法
echo "对n个数求和双链路算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 2 22 \
    2> perf_results/sum_array_double_perf.txt

# 5. 使用perf测试n个数求和 - 递归算法
echo "对n个数求和递归算法进行性能分析..."
perf stat -e cycles,instructions,branches,branch-misses \
    ./sum_array 3 22 \
    2> perf_results/sum_array_recursive_perf.txt

# 使用perf record记录详细数据

# 6. 矩阵向量内积 - 平凡算法
echo "记录矩阵向量内积平凡算法详细性能数据..."
perf record -g -o perf_results/matrix_vector_normal.data ./matrix_vector 1 2000 2000
perf report -i perf_results/matrix_vector_normal.data --stdio > perf_results/matrix_vector_normal_report.txt

# 7. 矩阵向量内积 - cache优化算法
echo "记录矩阵向量内积cache优化算法详细性能数据..."
perf record -g -o perf_results/matrix_vector_cache.data ./matrix_vector 2 2000 2000
perf report -i perf_results/matrix_vector_cache.data --stdio > perf_results/matrix_vector_cache_report.txt

echo "测试完成！结果保存在perf_results目录中"
ls -l perf_results/ 