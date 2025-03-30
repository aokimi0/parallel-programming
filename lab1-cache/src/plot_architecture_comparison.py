#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import os
import matplotlib
matplotlib.use('Agg')  # 非交互式后端
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import seaborn as sns

# 设置全局字体和样式
plt.rcParams['font.sans-serif'] = ['DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['axes.titlesize'] = 16
plt.rcParams['xtick.labelsize'] = 12
plt.rcParams['ytick.labelsize'] = 12
plt.rcParams['legend.fontsize'] = 12
plt.rcParams['figure.titlesize'] = 18
plt.rcParams['figure.figsize'] = (10, 6)
plt.style.use('seaborn-v0_8-paper')

# 确保fig目录存在
if not os.path.exists('fig'):
    os.makedirs('fig')

def plot_architecture_comparison():
    """生成x86与ARM架构性能比较的图表"""
    
    # 检查是否存在ARM实验结果
    if not (os.path.exists('results/matrix_vector_results_arm.csv') and 
            os.path.exists('results/sum_array_results_arm.csv')):
        print("警告：ARM实验结果未找到，生成错误提示图表")
        
        # 创建一个错误提示图表，而不是直接返回
        fig = plt.figure(figsize=(10, 6))
        ax = fig.add_subplot(111)
        
        # 添加错误文本
        ax.text(0.5, 0.5, 
                "无法生成架构比较图表\n\n缺少ARM实验数据\n\n请先运行ARM实验: ./run_complete_experiment.sh -r arm",
                horizontalalignment='center',
                verticalalignment='center',
                fontsize=16,
                transform=ax.transAxes)
        
        # 隐藏坐标轴
        ax.axis('off')
        
        # 保存图表
        plt.savefig('fig/architecture_comparison.png', dpi=300, bbox_inches='tight')
        plt.savefig('fig/matrix_arch_comparison.png', dpi=300, bbox_inches='tight')
        plt.savefig('fig/sum_arch_comparison.png', dpi=300, bbox_inches='tight')
        plt.close(fig)
        
        print("已生成架构比较错误提示图表")
        return
    
    # 读取x86和ARM的矩阵向量乘法数据
    df_x86 = pd.read_csv('results/matrix_vector_results.csv')
    df_arm = pd.read_csv('results/matrix_vector_results_arm.csv')
    
    # 合并数据以便比较
    df_x86['Architecture'] = 'x86'
    df_arm['Architecture'] = 'ARM'
    df_matrix = pd.concat([df_x86, df_arm])
    
    # 1. 矩阵向量乘法架构比较图
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    
    # 使用x86和ARM的相同矩阵大小（取交集）
    common_sizes = list(set(df_x86['Matrix Size']).intersection(set(df_arm['Matrix Size'])))
    common_sizes.sort()
    
    df_common = df_matrix[df_matrix['Matrix Size'].isin(common_sizes)]
    
    # 分组数据
    x86_naive = df_common[(df_common['Architecture'] == 'x86')]['Naive Algorithm (ms)']
    x86_cache = df_common[(df_common['Architecture'] == 'x86')]['Cache Optimized Algorithm (ms)']
    arm_naive = df_common[(df_common['Architecture'] == 'ARM')]['Naive Algorithm (ms)']
    arm_cache = df_common[(df_common['Architecture'] == 'ARM')]['Cache Optimized Algorithm (ms)']
    
    # 计算加速比
    x86_speedup = df_common[(df_common['Architecture'] == 'x86')]['Speedup']
    arm_speedup = df_common[(df_common['Architecture'] == 'ARM')]['Speedup']
    
    # 绘制执行时间对比图
    width = 0.2
    x = np.arange(len(common_sizes))
    
    ax1.bar(x - width*1.5, x86_naive, width, label='x86 Naive', color='#3274A1')
    ax1.bar(x - width/2, x86_cache, width, label='x86 Cache Opt', color='#E1812C')
    ax1.bar(x + width/2, arm_naive, width, label='ARM Naive', color='#3A923A')
    ax1.bar(x + width*1.5, arm_cache, width, label='ARM Cache Opt', color='#C03D3E')
    
    ax1.set_xlabel('Matrix Size')
    ax1.set_ylabel('Execution Time (ms)')
    ax1.set_title('Matrix-Vector Multiplication: x86 vs ARM')
    ax1.set_xticks(x)
    ax1.set_xticklabels(common_sizes)
    ax1.legend()
    ax1.grid(True, linestyle='--', alpha=0.7)
    
    # 绘制加速比对比图
    ax2.bar(x - width/2, x86_speedup, width, label='x86 Speedup', color='#3274A1')
    ax2.bar(x + width/2, arm_speedup, width, label='ARM Speedup', color='#3A923A')
    
    ax2.set_xlabel('Matrix Size')
    ax2.set_ylabel('Speedup Ratio')
    ax2.set_title('Optimization Speedup: x86 vs ARM')
    ax2.set_xticks(x)
    ax2.set_xticklabels(common_sizes)
    ax2.legend()
    ax2.grid(True, linestyle='--', alpha=0.7)
    
    # 添加数据标签
    for i, v in enumerate(x86_speedup):
        ax2.text(i - width/2, v + 0.1, f"{v:.2f}x", ha='center')
    for i, v in enumerate(arm_speedup):
        ax2.text(i + width/2, v + 0.1, f"{v:.2f}x", ha='center')
    
    plt.tight_layout()
    plt.savefig('fig/matrix_arch_comparison.png', dpi=300, bbox_inches='tight')
    plt.close(fig)
    
    # 2. 数组求和架构比较图
    # 读取x86和ARM的数组求和数据
    df_x86_sum = pd.read_csv('results/sum_array_results.csv')
    df_arm_sum = pd.read_csv('results/sum_array_results_arm.csv')
    
    # 合并数据以便比较
    df_x86_sum['Architecture'] = 'x86'
    df_arm_sum['Architecture'] = 'ARM'
    df_sum = pd.concat([df_x86_sum, df_arm_sum])
    
    # 使用x86和ARM的相同数组大小（取交集）
    common_sizes_sum = list(set(df_x86_sum['Array Size']).intersection(set(df_arm_sum['Array Size'])))
    common_sizes_sum.sort()
    
    df_common_sum = df_sum[df_sum['Array Size'].isin(common_sizes_sum)]
    
    # 将数组大小转换为2的幂形式的标签
    size_labels = [f"2^{int(np.log2(size))}" for size in common_sizes_sum]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    
    # 绘制执行时间对比图
    width = 0.15
    x = np.arange(len(common_sizes_sum))
    
    ax1.bar(x - width*2, df_common_sum[df_common_sum['Architecture'] == 'x86']['Naive Algorithm (ms)'], 
            width, label='x86 Naive', color='#3274A1')
    ax1.bar(x - width, df_common_sum[df_common_sum['Architecture'] == 'x86']['Dual Path Algorithm (ms)'], 
            width, label='x86 Dual Path', color='#E1812C')
    ax1.bar(x, df_common_sum[df_common_sum['Architecture'] == 'x86']['Recursive Algorithm (ms)'], 
            width, label='x86 Recursive', color='#3A923A')
    ax1.bar(x + width, df_common_sum[df_common_sum['Architecture'] == 'ARM']['Naive Algorithm (ms)'], 
            width, label='ARM Naive', color='#C03D3E')
    ax1.bar(x + width*2, df_common_sum[df_common_sum['Architecture'] == 'ARM']['Dual Path Algorithm (ms)'], 
            width, label='ARM Dual Path', color='#9372B2')
    ax1.bar(x + width*3, df_common_sum[df_common_sum['Architecture'] == 'ARM']['Recursive Algorithm (ms)'], 
            width, label='ARM Recursive', color='#845B53')
    
    ax1.set_xlabel('Array Size')
    ax1.set_ylabel('Execution Time (ms)')
    ax1.set_title('Array Sum: x86 vs ARM')
    ax1.set_yscale('log')  # 使用对数刻度更好地显示差异
    ax1.set_xticks(x)
    ax1.set_xticklabels(size_labels)
    ax1.legend()
    ax1.grid(True, linestyle='--', alpha=0.7)
    
    # 绘制加速比对比图
    ax2.bar(x - width*1.5, df_common_sum[df_common_sum['Architecture'] == 'x86']['Dual Path Speedup'], 
            width, label='x86 Dual Path', color='#3274A1')
    ax2.bar(x - width/2, df_common_sum[df_common_sum['Architecture'] == 'x86']['Recursive Speedup'], 
            width, label='x86 Recursive', color='#E1812C')
    ax2.bar(x + width/2, df_common_sum[df_common_sum['Architecture'] == 'ARM']['Dual Path Speedup'], 
            width, label='ARM Dual Path', color='#3A923A')
    ax2.bar(x + width*1.5, df_common_sum[df_common_sum['Architecture'] == 'ARM']['Recursive Speedup'], 
            width, label='ARM Recursive', color='#C03D3E')
    
    ax2.set_xlabel('Array Size')
    ax2.set_ylabel('Speedup Ratio')
    ax2.set_title('Algorithm Speedup: x86 vs ARM')
    ax2.set_xticks(x)
    ax2.set_xticklabels(size_labels)
    ax2.legend()
    ax2.grid(True, linestyle='--', alpha=0.7)
    
    plt.tight_layout()
    plt.savefig('fig/sum_arch_comparison.png', dpi=300, bbox_inches='tight')
    plt.close(fig)
    
    # 3. 架构特性对比图
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, polar=True)
    
    # 架构特性数据
    architectures = ['x86-64', 'ARM (aarch64)']
    features = {
        'Cache Line Size': [64, 64],  # bytes
        'L1 Cache Size': [48+32, 32+32],  # KB (I-Cache + D-Cache)
        'L2 Cache Size': [1280, 512],  # KB
        'L3 Cache Size': [18432, 4096],  # KB
        'Branch Predictor': [9, 7],  # 相对复杂度，1-10
        'Out-of-Order Execution': [10, 8],  # 相对复杂度，1-10
        'SIMD Width': [256, 128]  # bits (AVX2 vs NEON)
    }
    
    # 准备雷达图数据
    categories = list(features.keys())
    N = len(categories)
    
    # 计算雷达图的角度
    angles = [n / float(N) * 2 * np.pi for n in range(N)]
    angles += angles[:1]  # 闭合多边形
    
    # 标准化数据为0-1之间
    normalized_data = {}
    for k, v in features.items():
        max_val = max(v)
        normalized_data[k] = [val/max_val for val in v]
    
    # 准备绘图数据
    values_x86 = [normalized_data[category][0] for category in categories]
    values_arm = [normalized_data[category][1] for category in categories]
    
    # 闭合多边形
    values_x86 += values_x86[:1]
    values_arm += values_arm[:1]
    
    # 设置角度
    ax.set_theta_offset(np.pi / 2)
    ax.set_theta_direction(-1)
    
    # 绘制多边形和数据点
    ax.plot(angles, values_x86, 'o-', linewidth=2, label='x86-64')
    ax.plot(angles, values_arm, 'o-', linewidth=2, label='ARM (aarch64)')
    ax.fill(angles, values_x86, alpha=0.25)
    ax.fill(angles, values_arm, alpha=0.25)
    
    # 设置分类标签位置
    ax.set_xticks(angles[:-1])
    ax.set_xticklabels(categories)
    
    # 添加图例和标题
    ax.legend(loc='upper right')
    ax.set_title("Architecture Feature Comparison (Normalized)")
    
    plt.tight_layout()
    plt.savefig('fig/architecture_comparison.png', dpi=300, bbox_inches='tight')
    plt.close(fig)
    
    print("Architecture comparison charts generated successfully")
    
if __name__ == "__main__":
    plot_architecture_comparison() 