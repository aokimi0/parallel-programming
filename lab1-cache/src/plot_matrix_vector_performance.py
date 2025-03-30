#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import os
import re

# 创建图片目录
if not os.path.exists('fig'):
    os.makedirs('fig')

# 从实验结果文件中读取数据
def read_matrix_vector_data():
    data = {}
    sizes = [1000, 2000, 4000]
    
    for size in sizes:
        try:
            with open(f'results/matrix_vector_{size}.txt', 'r') as f:
                lines = f.readlines()
                
            size_data = {}
            for line in lines:
                match = re.search(r'(.*) time: ([0-9.]+) ms', line)
                if match:
                    method = match.group(1).strip()
                    time_ms = float(match.group(2))
                    size_data[method] = time_ms
            
            if size_data:
                data[size] = size_data
                print(f"Loaded data for size {size}: {size_data}")
        except (FileNotFoundError, IOError) as e:
            print(f"Warning: Could not read data for size {size}: {e}")
    
    return data

def plot_matrix_vector_performance():
    # 读取实验数据
    data = read_matrix_vector_data()
    
    if not data:
        print("No data found. Using simulated data.")
        # 使用模拟数据
        data = {
            1000: {
                'Col access': 2.24, 'Row access': 0.94, 
                'Unroll5': 0.66, 'Unroll10': 0.49, 'Unroll15': 1.07, 'Unroll20': 1.27
            },
            2000: {
                'Col access': 14.02, 'Row access': 2.80,
                'Unroll5': 2.48, 'Unroll10': 3.85, 'Unroll15': 3.47, 'Unroll20': 2.90
            },
            4000: {
                'Col access': 123.36, 'Row access': 17.09,
                'Unroll5': 13.77, 'Unroll10': 10.13, 'Unroll15': 13.14, 'Unroll20': 12.80
            }
        }
    
    # 提取数据
    sizes = sorted(data.keys())
    methods = ['Col access', 'Row access', 'Unroll5', 'Unroll10', 'Unroll15', 'Unroll20']
    
    # 检查哪些方法在所有尺寸中都有数据
    valid_methods = []
    for method in methods:
        valid = True
        for size in sizes:
            if method not in data[size]:
                valid = False
                print(f"Method {method} missing for size {size}")
                break
        if valid:
            valid_methods.append(method)
    
    if not valid_methods:
        print("No valid methods found across all sizes.")
        return
    
    # 获取每种方法在不同矩阵尺寸下的执行时间
    perf_data = {}
    for method in valid_methods:
        perf_data[method] = [data[size][method] for size in sizes]
    
    # 计算加速比（相对于列访问的改进）
    speedup_data = {}
    for method in valid_methods:
        if method != 'Col access':
            speedup_data[method] = [data[size]['Col access'] / data[size][method] for size in sizes]
    
    # 创建两个子图
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 颜色设置
    colors = {
        'Col access': 'royalblue',
        'Row access': 'darkorange',
        'Unroll5': 'forestgreen',
        'Unroll10': 'firebrick',
        'Unroll15': 'purple',
        'Unroll20': 'brown'
    }
    
    # 1. 左图：对数刻度的执行时间
    for method in valid_methods:
        ax1.plot(sizes, perf_data[method], 'o-', linewidth=2, markersize=8, 
                label=method, color=colors.get(method, 'gray'))
    
    ax1.set_xlabel('Matrix Size (n×n)', fontsize=12)
    ax1.set_ylabel('Execution Time (ms, log scale)', fontsize=12)
    ax1.set_title('Matrix-Vector Multiplication Performance', fontsize=14)
    ax1.set_yscale('log')
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend(loc='upper left')
    
    # 在数据点上添加值标签
    for method in valid_methods:
        for i, (size, time) in enumerate(zip(sizes, perf_data[method])):
            ax1.annotate(f'{time:.2f}', 
                        xy=(size, time),
                        xytext=(0, 10 if method == 'Col access' else -15),
                        textcoords='offset points',
                        ha='center', fontsize=8)
    
    # 2. 右图：加速比（相对于列访问）
    for method in speedup_data:
        ax2.plot(sizes, speedup_data[method], 'o-', linewidth=2, markersize=8,
                label=method, color=colors.get(method, 'gray'))
    
    ax2.set_xlabel('Matrix Size (n×n)', fontsize=12)
    ax2.set_ylabel('Speedup (vs Column Access)', fontsize=12)
    ax2.set_title('Performance Speedup vs Column Access', fontsize=14)
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend(loc='upper left')
    
    # 在数据点上添加值标签
    for method in speedup_data:
        for i, (size, speedup) in enumerate(zip(sizes, speedup_data[method])):
            ax2.annotate(f'{speedup:.1f}x', 
                        xy=(size, speedup),
                        xytext=(0, 5),
                        textcoords='offset points',
                        ha='center', fontsize=8)
    
    # 调整布局并保存
    plt.tight_layout()
    plt.savefig('fig/matrix_vector_performance.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Matrix-vector performance chart saved as fig/matrix_vector_performance.png")

    # 绘制4000x4000矩阵的详细性能对比柱状图
    plot_detail_performance(data)

def plot_detail_performance(data):
    if 4000 not in data:
        print("No data for 4000x4000 matrix. Skipping detailed performance chart.")
        return
    
    size_data = data[4000]
    methods = ['Col access', 'Row access', 'Unroll5', 'Unroll10', 'Unroll15', 'Unroll20']
    
    # 过滤出存在的方法
    valid_methods = [m for m in methods if m in size_data]
    times = [size_data[m] for m in valid_methods]
    
    # 计算相对于列访问的性能提升百分比
    col_time = size_data.get('Col access', 0)
    if col_time > 0:
        improvements = [(col_time - t) / col_time * 100 for t in times]
    else:
        improvements = [0] * len(valid_methods)
    
    # 创建图表
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # 颜色设置
    colors = {
        'Col access': 'royalblue',
        'Row access': 'darkorange',
        'Unroll5': 'forestgreen',
        'Unroll10': 'firebrick',
        'Unroll15': 'purple',
        'Unroll20': 'brown'
    }
    
    bar_colors = [colors.get(m, 'gray') for m in valid_methods]
    
    # 1. 左图：执行时间柱状图
    bars1 = ax1.bar(valid_methods, times, color=bar_colors, width=0.6)
    
    # 添加数据标签
    for bar in bars1:
        height = bar.get_height()
        ax1.annotate(f'{height:.2f} ms',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9)
    
    ax1.set_title('Matrix-Vector Multiplication Time (4000×4000)', fontsize=14)
    ax1.set_ylabel('Execution Time (ms)', fontsize=12)
    ax1.set_xlabel('Method', fontsize=12)
    
    # 如果列访问时间太长，截断y轴以便更好地查看其他方法
    if 'Col access' in size_data and size_data['Col access'] > 60:
        other_max = max([t for m, t in size_data.items() if m != 'Col access'])
        ax1.set_ylim(0, other_max * 1.5)
        # 添加截断标记
        ax1.plot([bars1[0].get_x() - 0.1, bars1[0].get_x() + bars1[0].get_width() + 0.1], 
                [other_max * 1.5, other_max * 1.5], 'k-', lw=1)
        ax1.plot([bars1[0].get_x() - 0.1, bars1[0].get_x() + bars1[0].get_width() + 0.1], 
                [other_max * 1.45, other_max * 1.45], 'k-', lw=1)
        # 添加完整值的标注
        ax1.annotate(f'Full value: {size_data["Col access"]:.2f} ms', 
                    xy=(0, other_max * 1.3),
                    xytext=(5, -5),
                    textcoords='offset points',
                    ha='left', fontsize=8)
    
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 2. 右图：性能提升百分比
    # 过滤掉第一个方法（Col access）的改进百分比
    if 'Col access' in valid_methods and valid_methods[0] == 'Col access':
        improvement_methods = valid_methods[1:]
        improvement_values = improvements[1:]
    else:
        improvement_methods = valid_methods
        improvement_values = improvements
    
    bars2 = ax2.bar(improvement_methods, improvement_values, 
                  color=[colors.get(m, 'gray') for m in improvement_methods],
                  width=0.6)
    
    # 添加数据标签
    for bar in bars2:
        height = bar.get_height()
        ax2.annotate(f'{height:.1f}%',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9)
    
    ax2.set_title('Performance Improvement vs Column Access (4000×4000)', fontsize=14)
    ax2.set_ylabel('Improvement (%)', fontsize=12)
    ax2.set_xlabel('Method', fontsize=12)
    ax2.grid(axis='y', linestyle='--', alpha=0.7)
    
    # 添加水平基准线
    ax2.axhline(y=0, color='black', linestyle='-', alpha=0.2)
    
    # 调整布局并保存
    plt.tight_layout()
    plt.savefig('fig/matrix_vector_detail_performance.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Detailed matrix-vector performance chart saved as fig/matrix_vector_detail_performance.png")

if __name__ == "__main__":
    plot_matrix_vector_performance() 