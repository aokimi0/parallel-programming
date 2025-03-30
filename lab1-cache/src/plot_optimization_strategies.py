#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import os
import pandas as pd

# 创建图片目录
if not os.path.exists('fig'):
    os.makedirs('fig')

# 使用报告中表格数据重新生成数组求和性能图 (图10)
def plot_sum_array_performance_from_table():
    # 使用报告中的实际表格数据
    sizes = [2**18, 2**22, 2**24]
    size_labels = ['2^18 (1M)', '2^22 (4M)', '2^24 (16M)']
    
    # 表格中的数据
    naive_times = [1.21, 3.53, 16.39]
    dual_times = [2.39, 3.70, 10.49]
    recursive_times = [2.03, 5.47, 45.25]
    
    # 计算加速比
    dual_speedups = [naive_times[i]/dual_times[i] for i in range(len(sizes))]
    recursive_speedups = [naive_times[i]/recursive_times[i] for i in range(len(sizes))]
    
    # 创建两栏布局图表
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 左边：执行时间比较
    width = 0.25
    x = np.arange(len(sizes))
    
    bars1 = ax1.bar(x - width, naive_times, width, label='Naive Algorithm', color='#1f77b4')
    bars2 = ax1.bar(x, dual_times, width, label='Dual Path Algorithm', color='#ff7f0e')
    bars3 = ax1.bar(x + width, recursive_times, width, label='Recursive Algorithm', color='#2ca02c')
    
    # 添加数据标签
    def add_labels(bars):
        for bar in bars:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height + 0.5,
                    f'{height:.2f}', ha='center', va='bottom', fontsize=9)
    
    add_labels(bars1)
    add_labels(bars2)
    add_labels(bars3)
    
    ax1.set_ylabel('Execution Time (ms)', fontsize=12)
    ax1.set_xticks(x)
    ax1.set_xticklabels(size_labels)
    ax1.set_xlabel('Array Size', fontsize=12)
    ax1.set_title('Execution Time Comparison', fontsize=14)
    ax1.legend()
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 右边：加速比
    width = 0.3
    x = np.arange(len(sizes))
    
    bars4 = ax2.bar(x - width/2, dual_speedups, width, label='Dual Path Speedup', color='#ff7f0e')
    bars5 = ax2.bar(x + width/2, recursive_speedups, width, label='Recursive Speedup', color='#2ca02c')
    
    # 添加数据标签
    add_labels(bars4)
    add_labels(bars5)
    
    # 添加水平参考线(y=1)表示与朴素算法相同性能
    ax2.axhline(y=1.0, color='gray', linestyle='--', alpha=0.7)
    
    ax2.set_ylabel('Speedup Ratio', fontsize=12)
    ax2.set_xticks(x)
    ax2.set_xticklabels(size_labels)
    ax2.set_xlabel('Array Size', fontsize=12)
    ax2.set_title('Algorithm Speedup Ratio', fontsize=14)
    ax2.legend()
    ax2.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 确保所有数据可见
    ax2.set_ylim(0, max(max(dual_speedups), max(recursive_speedups)) * 1.2)
    
    # 布局和保存
    plt.tight_layout()
    plt.savefig('fig/sum_array_performance.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Array sum performance chart saved as fig/sum_array_performance.png")

# 优化策略对比图 (图11)
def plot_optimization_strategies():
    try:
        # 尝试从结果文件中读取数据
        # 读取矩阵-向量乘法和数组求和实验数据
        df_matrix = pd.read_csv('results/matrix_vector_results.csv')
        df_sum = pd.read_csv('results/sum_array_results.csv')
        
        # 计算平均加速比
        matrix_speedups = df_matrix['Speedup'].tolist()
        matrix_avg_speedup = sum(matrix_speedups) / len(matrix_speedups)
        
        sum_dual_speedups = df_sum['Dual Path Speedup'].tolist()
        sum_avg_dual_speedup = sum(sum_dual_speedups) / len(sum_dual_speedups)
        
        # 将计算结果应用到图表
        speedups = [matrix_avg_speedup, sum_avg_dual_speedup]
        
    except (FileNotFoundError, pd.errors.EmptyDataError):
        print("警告：优化实验结果数据未找到，使用默认数据")
        # 使用默认值
        speedups = [3.5, 1.5]  # 平均加速比
    
    # 创建2x1子图布局
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 左图：性能提升水平
    opt_types = ['Spatial Locality', 'Instruction-Level\nParallelism']
    
    bars = ax1.bar(opt_types, speedups, color=['steelblue', 'darkorange'], width=0.5)
    
    # 在柱状图上添加数值标签
    for bar in bars:
        height = bar.get_height()
        ax1.annotate(f'{height:.2f}x',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),  # 3点垂直偏移
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=12)
    
    ax1.set_ylim(0, max(speedups) * 1.2)  # 动态设置Y轴上限
    ax1.set_ylabel('Average Speedup', fontsize=12)
    ax1.set_title('Performance Improvement by Optimization Type', fontsize=14)
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 右图：优化策略特性雷达图
    categories = ['Applicability', 'Implementation\nDifficulty', 'Stability', 'Hardware\nDependency', 'Compiler\nSupport']
    
    # 数据：值越高越好，除了实现难度和硬件相关性（值越低越好）
    cache_values = [4, 2, 4, 3, 3]  # 空间局部性优化
    ilp_values = [3, 3, 3, 4, 4]    # 指令级并行优化
    
    # 设置雷达图
    angles = np.linspace(0, 2*np.pi, len(categories), endpoint=False).tolist()
    angles += angles[:1]  # 闭合雷达图
    
    categories = np.array(categories)
    cache_values = np.array(cache_values)
    ilp_values = np.array(ilp_values)
    
    # 闭合数据
    cache_values = np.concatenate((cache_values, [cache_values[0]]))
    ilp_values = np.concatenate((ilp_values, [ilp_values[0]]))
    
    # 绘制雷达图
    ax2.plot(angles, cache_values, 'b-', linewidth=2, label='Spatial Locality')
    ax2.plot(angles, ilp_values, 'orange', linewidth=2, label='ILP')
    ax2.fill(angles, cache_values, 'b', alpha=0.1)
    ax2.fill(angles, ilp_values, 'orange', alpha=0.1)
    
    # 标签设置
    ax2.set_xticks(angles[:-1])
    ax2.set_xticklabels(categories, fontsize=10)
    ax2.set_yticks(np.arange(1, 6))
    ax2.set_ylim(0, 5)
    ax2.set_title('Optimization Strategy Characteristics', fontsize=14)
    ax2.legend(loc='upper right', bbox_to_anchor=(0.1, 0.1))
    
    # 布局和保存
    plt.tight_layout()
    plt.savefig('fig/optimization_strategies_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Optimization strategies comparison chart saved as fig/optimization_strategies_comparison.png")

# 数组求和架构比较图 (图10)
def plot_array_sum_arch_comparison():
    try:
        # 检查是否存在x86和ARM数据文件
        if not (os.path.exists('results/sum_array_results.csv') and 
                os.path.exists('results/sum_array_results_arm.csv')):
            raise FileNotFoundError("缺少数组求和架构比较所需数据文件")
        
        # 读取x86和ARM的数据
        df_x86 = pd.read_csv('results/sum_array_results.csv')
        df_arm = pd.read_csv('results/sum_array_results_arm.csv')
        
        # 获取共同的数组大小（取交集）
        common_sizes = list(set(df_x86['Array Size']).intersection(set(df_arm['Array Size'])))
        common_sizes.sort()
        
        if len(common_sizes) < 2:
            raise ValueError("x86和ARM数据中没有足够的共同数组大小")
        
        # 按大小过滤数据
        df_x86_filtered = df_x86[df_x86['Array Size'].isin(common_sizes)]
        df_arm_filtered = df_arm[df_arm['Array Size'].isin(common_sizes)]
        
        # 提取数据
        sizes = common_sizes
        size_labels = [f'2^{int(np.log2(size))}' for size in sizes]
        
        x86_naive = df_x86_filtered['Naive Algorithm (ms)'].tolist()
        x86_dual = df_x86_filtered['Dual Path Algorithm (ms)'].tolist()
        x86_recursive = df_x86_filtered['Recursive Algorithm (ms)'].tolist()
        
        arm_naive = df_arm_filtered['Naive Algorithm (ms)'].tolist()
        arm_dual = df_arm_filtered['Dual Path Algorithm (ms)'].tolist()
        arm_recursive = df_arm_filtered['Recursive Algorithm (ms)'].tolist()
        
    except (FileNotFoundError, pd.errors.EmptyDataError, ValueError) as e:
        print(f"警告：无法读取架构比较数据: {e}，使用默认数据")
        # 使用默认数据
        sizes = [2**18, 2**19, 2**20, 2**21]
        size_labels = ['2^18', '2^19', '2^20', '2^21']
        
        # x86架构数据
        x86_naive = [0.59, 2.28, 2.67, 5.68]
        x86_dual = [0.35, 1.18, 1.78, 3.81]
        x86_recursive = [1.29, 2.86, 5.59, 12.92]
        
        # ARM架构数据（估算值）
        arm_naive = [7.8, 26.4, 32.5, 74.03]
        arm_dual = [6.1, 15.3, 18.6, 40.2]
        arm_recursive = [13.5, 28.9, 52.4, 105.7]
    
    # 创建2x1子图布局
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 左图：性能对比（对数刻度）
    width = 0.2  # 柱宽度
    x = np.arange(len(sizes))
    
    # 绘制柱状图（对数刻度）
    ax1.bar(x - 0.3, x86_naive, width, label='x86 Naive', color='darkblue')
    ax1.bar(x - 0.1, x86_dual, width, label='x86 Dual-Path', color='blue')
    ax1.bar(x + 0.1, arm_naive, width, label='ARM Naive', color='darkred')
    ax1.bar(x + 0.3, arm_dual, width, label='ARM Dual-Path', color='red')
    
    ax1.set_yscale('log')
    ax1.set_ylabel('Execution Time (ms, log scale)', fontsize=12)
    ax1.set_xticks(x)
    ax1.set_xticklabels(size_labels)
    ax1.set_xlabel('Array Size', fontsize=12)
    ax1.set_title('Array Sum Performance by Architecture', fontsize=14)
    ax1.legend()
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 右图：加速比
    x86_speedup = [x86_naive[i]/x86_dual[i] for i in range(len(sizes))]
    arm_speedup = [arm_naive[i]/arm_dual[i] for i in range(len(sizes))]
    
    ax2.plot(size_labels, x86_speedup, 'bo-', linewidth=2, markersize=8, label='x86 Speedup')
    ax2.plot(size_labels, arm_speedup, 'ro-', linewidth=2, markersize=8, label='ARM Speedup')
    
    ax2.set_ylabel('Speedup (Naive/Dual-Path)', fontsize=12)
    ax2.set_xlabel('Array Size', fontsize=12)
    ax2.set_title('Dual-Path Algorithm Speedup by Architecture', fontsize=14)
    ax2.legend()
    ax2.grid(linestyle='--', alpha=0.7)
    ax2.set_ylim(1.0, max(max(x86_speedup), max(arm_speedup)) * 1.1)
    
    # 添加水平参考线
    ax2.axhline(y=1.0, color='gray', linestyle='-', alpha=0.3)
    
    # 布局和保存
    plt.tight_layout()
    plt.savefig('fig/array_sum_arch_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Array sum architecture comparison chart saved as fig/array_sum_arch_comparison.png")

if __name__ == "__main__":
    plot_sum_array_performance_from_table() # 使用表格数据绘制图表
    plot_optimization_strategies()
    plot_array_sum_arch_comparison() 