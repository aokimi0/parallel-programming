#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import os
import re

# 创建图片目录
if not os.path.exists('fig'):
    os.makedirs('fig')

def plot_loop_unrolling_performance():
    # 使用4000×4000矩阵的实际测量数据
    try:
        # 尝试读取实际测量数据
        with open('results/matrix_vector_4000.txt', 'r') as f:
            lines = f.readlines()
            
        methods = []
        times = []
        
        # 解析文件中的数据
        for line in lines:
            match = re.search(r'(.*) time: ([0-9.]+) ms', line)
            if match:
                method = match.group(1).strip()
                time_ms = float(match.group(2))
                methods.append(method)
                times.append(time_ms)
        
        print(f"Parsed data: {list(zip(methods, times))}")
    
    except (FileNotFoundError, IOError) as e:
        print(f"Warning: Could not read actual data: {e}. Using default values.")
        # 如果无法读取实际数据，使用最后实验的结果
        methods = ['Col access', 'Row access', 'Unroll5', 'Unroll10', 'Unroll15', 'Unroll20']
        times = [123.36, 17.09, 13.77, 10.13, 13.14, 12.80]
    
    if not methods:
        print("No data found. Using hardcoded values.")
        methods = ['Col access', 'Row access', 'Unroll5', 'Unroll10', 'Unroll15', 'Unroll20']
        times = [123.36, 17.09, 13.77, 10.13, 13.14, 12.80]
        
    print(f"Using data: {list(zip(methods, times))}")
    
    # 计算相对于基准行访问的改进百分比
    base_time = None
    for i, method in enumerate(methods):
        if method == 'Row access':
            base_time = times[i]
            break
    
    if base_time is None:
        print("Warning: Row access method not found. Using first non-column value as base.")
        for i, method in enumerate(methods):
            if method != 'Col access':
                base_time = times[i]
                break
        if base_time is None and len(times) > 0:
            base_time = times[0]
    
    # 只保留行访问和循环展开方法，过滤掉列访问
    row_methods = []
    row_times = []
    for i, method in enumerate(methods):
        if method != 'Col access':
            row_methods.append(method)
            row_times.append(times[i])
    
    if not row_methods:
        print("No row methods found. Cannot generate chart.")
        return
        
    improvements = [((base_time - t) / base_time) * 100 for t in row_times]  # 正值表示性能提升
    
    # 创建图表
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 1. 左图：执行时间
    colors = ['darkorange', 'forestgreen', 'firebrick', 'purple', 'brown']
    # 确保colors数组足够长
    while len(colors) < len(row_methods):
        colors.extend(['gray', 'lightblue', 'lightgreen'])
    
    bars = ax1.bar(row_methods, row_times, color=colors[:len(row_methods)], width=0.6)
    
    # 添加数据标签
    for bar in bars:
        height = bar.get_height()
        ax1.annotate(f'{height:.2f} ms',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9)
    
    ax1.set_title('Execution Time by Unrolling Factor (4000×4000)', fontsize=12)
    ax1.set_ylabel('Execution Time (ms)', fontsize=11)
    ax1.set_xlabel('Method', fontsize=11)
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 2. 右图：性能改进百分比
    imp_colors = ['firebrick' if i < 0 else 'forestgreen' for i in improvements]
    bars2 = ax2.bar(row_methods, improvements, color=imp_colors, width=0.6)
    
    # 添加数据标签
    for bar in bars2:
        height = bar.get_height()
        if height < 0:
            va = 'top'
            offset = -15
        else:
            va = 'bottom'
            offset = 3
        
        ax2.annotate(f'{height:.1f}%',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, offset),
                    textcoords="offset points",
                    ha='center', va=va, fontsize=9)
    
    ax2.set_title('Performance Improvement from Loop Unrolling (4000×4000)', fontsize=12)
    ax2.set_ylabel('Performance Improvement (%)', fontsize=11)
    ax2.set_xlabel('Method', fontsize=11)
    ax2.axhline(y=0, color='black', linestyle='-', alpha=0.3)
    
    # 设置y轴范围，保证显示效果
    max_imp = max(abs(min(improvements)), abs(max(improvements))) * 1.2
    ax2.set_ylim(-max_imp, max_imp)
    ax2.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 添加说明性注释
    fig.text(0.95, 0.05, 'Based on actual measurements\nPerformance varies with unrolling factor',
             ha='right', fontsize=8)
    
    # 保存图表
    plt.tight_layout()
    plt.savefig('fig/loop_unrolling_performance.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Loop unrolling performance chart saved as fig/loop_unrolling_performance.png")

if __name__ == "__main__":
    plot_loop_unrolling_performance() 