#!/bin/bash
set -e

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 显示分隔线和标题
function print_section {
    echo -e "\n${GREEN}======================= $1 =======================${NC}\n"
}

# 创建和清理工作目录
setup_workspace() {
    print_section "创建和清理工作目录"
    
    mkdir -p results
    mkdir -p fig
    mkdir -p cachegrind_logs
    mkdir -p bin
    mkdir -p arm_build
}

# 编译程序
compile_programs() {
    print_section "编译实验程序"
    
    g++ -O0 src/matrix_vector.cpp -o bin/matrix_vector
    g++ -O0 src/sum_array.cpp -o bin/sum_array
    echo -e "${BLUE}编译完成${NC}"
    
    # 为ARM编译
    echo -e "${BLUE}为ARM架构编译...${NC}"
    if command -v aarch64-linux-gnu-g++ &> /dev/null; then
        aarch64-linux-gnu-g++ -O0 src/matrix_vector.cpp -o arm_build/matrix_vector_arm
        aarch64-linux-gnu-g++ -O0 src/sum_array.cpp -o arm_build/sum_array_arm
        echo -e "${BLUE}ARM编译完成${NC}"
    else
        echo -e "${RED}未找到ARM交叉编译器，跳过ARM编译${NC}"
    fi
}

# 运行矩阵-向量乘法测试
run_matrix_vector_test() {
    print_section "运行矩阵-向量基本测试"
    # 测试不同大小矩阵
    sizes=(1000 2000 4000)
    echo "Matrix Size,Naive Algorithm (ms),Cache Optimized Algorithm (ms),Speedup" > results/matrix_vector_results.csv

    for size in "${sizes[@]}"; do
        echo -e "${BLUE}测试矩阵大小: ${size}x${size}${NC}"
        
        # 运行并收集所有方法的结果
        output=$(bin/matrix_vector $size 0)
        
        col_time=$(echo "$output" | grep "Col access time" | awk '{print $4}')
        row_time=$(echo "$output" | grep "Row access time" | awk '{print $4}')
        unroll5_time=$(echo "$output" | grep "Unroll5 time" | awk '{print $3}')
        unroll10_time=$(echo "$output" | grep "Unroll10 time" | awk '{print $3}')
        unroll15_time=$(echo "$output" | grep "Unroll15 time" | awk '{print $3}')
        unroll20_time=$(echo "$output" | grep "Unroll20 time" | awk '{print $3}')
        
        # 计算加速比
        speedup=$(echo "scale=2; $col_time/$row_time" | bc)
        
        echo "$size,$col_time,$row_time,$speedup" >> results/matrix_vector_results.csv
        
        echo -e "  列访问: ${col_time}ms, 行访问: ${row_time}ms"
        echo -e "  unroll5: ${unroll5_time}ms, unroll10: ${unroll10_time}ms"
        echo -e "  unroll15: ${unroll15_time}ms, unroll20: ${unroll20_time}ms"
    done
}

# 运行ARM架构矩阵向量乘法测试
run_arm_matrix_vector_test() {
    print_section "运行ARM架构矩阵-向量测试"
    
    if ! command -v qemu-aarch64 &> /dev/null || [ ! -f arm_build/matrix_vector_arm ]; then
        echo -e "${RED}未找到QEMU或ARM二进制文件，跳过ARM测试${NC}"
        return
    fi
    
    sizes=(1000 2000 4000)
    echo "Matrix Size,Naive Algorithm (ms),Cache Optimized Algorithm (ms),Speedup" > results/matrix_vector_results_arm.csv

    for size in "${sizes[@]}"; do
        echo -e "${BLUE}测试ARM架构，矩阵大小: ${size}x${size}${NC}"
        
        # 运行ARM版本
        output=$(qemu-aarch64 -L /usr/aarch64-linux-gnu ./arm_build/matrix_vector_arm $size 0)
        
        col_time=$(echo "$output" | grep "Col access time" | awk '{print $4}')
        row_time=$(echo "$output" | grep "Row access time" | awk '{print $4}')
        
        # 计算加速比
        speedup=$(echo "scale=2; $col_time/$row_time" | bc)
        
        echo "$size,$col_time,$row_time,$speedup" >> results/matrix_vector_results_arm.csv
        
        echo -e "  ARM列访问: ${col_time}ms, ARM行访问: ${row_time}ms, 加速比: ${speedup}x"
    done
}

# 运行数组求和测试
run_sum_array_test() {
    print_section "运行数组求和测试"
    powers=(18 19 20 21)
    echo "Array Size,Naive Algorithm (ms),Dual Path Algorithm (ms),Recursive Algorithm (ms),Dual Path Speedup,Recursive Speedup" > results/sum_array_results.csv

    for power in "${powers[@]}"; do
        size=$((2**$power))
        echo -e "${BLUE}测试数组大小: 2^$power = $size${NC}"
        
        output=$(bin/sum_array $size)
        
        naive_time=$(echo "$output" | grep "Naive sum time" | awk '{print $4}')
        dual_time=$(echo "$output" | grep "Dual path sum time" | awk '{print $5}')
        recursive_time=$(echo "$output" | grep "Recursive sum time" | awk '{print $4}')
        
        dual_speedup=$(echo "scale=2; $naive_time/$dual_time" | bc)
        recursive_speedup=$(echo "scale=2; $naive_time/$recursive_time" | bc)
        
        echo "$size,$naive_time,$dual_time,$recursive_time,$dual_speedup,$recursive_speedup" >> results/sum_array_results.csv
        
        echo -e "  朴素算法: ${naive_time}ms, 双链路算法: ${dual_time}ms, 递归算法: ${recursive_time}ms"
        echo -e "  加速比(双链路): ${dual_speedup}x, 加速比(递归): ${recursive_speedup}x"
    done
}

# 运行ARM架构数组求和测试
run_arm_sum_array_test() {
    print_section "运行ARM架构数组求和测试"
    
    if ! command -v qemu-aarch64 &> /dev/null || [ ! -f arm_build/sum_array_arm ]; then
        echo -e "${RED}未找到QEMU或ARM二进制文件，跳过ARM测试${NC}"
        return
    fi
    
    powers=(18 19 20 21)
    echo "Array Size,Naive Algorithm (ms),Dual Path Algorithm (ms),Recursive Algorithm (ms),Dual Path Speedup,Recursive Speedup" > results/sum_array_results_arm.csv

    for power in "${powers[@]}"; do
        size=$((2**$power))
        echo -e "${BLUE}测试ARM架构，数组大小: 2^$power = $size${NC}"
        
        output=$(qemu-aarch64 -L /usr/aarch64-linux-gnu ./arm_build/sum_array_arm $size)
        
        naive_time=$(echo "$output" | grep "Naive sum time" | awk '{print $4}')
        dual_time=$(echo "$output" | grep "Dual path sum time" | awk '{print $5}')
        recursive_time=$(echo "$output" | grep "Recursive sum time" | awk '{print $4}')
        
        dual_speedup=$(echo "scale=2; $naive_time/$dual_time" | bc)
        recursive_speedup=$(echo "scale=2; $naive_time/$recursive_time" | bc)
        
        echo "$size,$naive_time,$dual_time,$recursive_time,$dual_speedup,$recursive_speedup" >> results/sum_array_results_arm.csv
        
        echo -e "  ARM朴素算法: ${naive_time}ms, 双链路算法: ${dual_time}ms, 递归算法: ${recursive_time}ms"
        echo -e "  加速比(双链路): ${dual_speedup}x, 加速比(递归): ${recursive_speedup}x"
    done
}

# 生成架构比较图表
generate_arch_comparison_plots() {
    print_section "生成架构比较图表"
    
    if [ -f src/plot_architecture_comparison.py ]; then
        python3 src/plot_architecture_comparison.py
        echo -e "${BLUE}架构比较图表生成完成${NC}"
    else
        echo -e "${RED}未找到plot_architecture_comparison.py，跳过图表生成${NC}"
    fi
}

# 主函数
main() {
    print_section "开始实验"
    
    # 准备工作目录
    setup_workspace
    
    # 编译程序
    compile_programs
    
    # 运行矩阵-向量乘法测试
    run_matrix_vector_test
    
    # 运行数组求和测试
    run_sum_array_test
    
    # 如果指定了ARM测试
    if [ "$1" == "--with-arm" ]; then
        run_arm_matrix_vector_test
        run_arm_sum_array_test
        generate_arch_comparison_plots
    fi
    
    print_section "实验完成"
    echo -e "数据保存在 ${BLUE}results/${NC} 目录"
    echo -e "图表保存在 ${BLUE}fig/${NC} 目录"
}

# 运行主函数
main "$@" 