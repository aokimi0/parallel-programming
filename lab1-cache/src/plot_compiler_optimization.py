import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import matplotlib as mpl

# 设置全局字体和样式
plt.style.use('ggplot')
mpl.rcParams['font.family'] = 'DejaVu Sans'
mpl.rcParams['axes.unicode_minus'] = False

# 确保输出目录存在
output_dir = os.environ.get('PLOT_DIR', 'fig')
os.makedirs(output_dir, exist_ok=True)

def set_figure_style():
    """设置图表样式为科技论文风格"""
    plt.figure(figsize=(10, 8), dpi=300)
    plt.tight_layout()
    return plt.gca()

def plot_matrix_vector_optimization():
    """绘制编译器优化级别对矩阵向量乘法的影响"""
    try:
        # 读取数据
        df = pd.read_csv('results/compiler_opt_matrix.csv')
        
        # 创建图表
        ax = set_figure_style()
        
        # 准备绘图数据
        opt_levels = df['Optimization Level'].tolist()
        col_times = df['Column Access'].astype(float).tolist()
        row_times = df['Row Access'].astype(float).tolist()
        speedups = df['Speedup'].astype(float).tolist()
        
        # 创建双轴图表
        ax1 = plt.subplot(111)
        
        # 左Y轴: 执行时间
        width = 0.35
        x = np.arange(len(opt_levels))
        bars1 = ax1.bar(x - width/2, col_times, width, label='Column Access', color='#1f77b4')
        bars2 = ax1.bar(x + width/2, row_times, width, label='Row Access', color='#ff7f0e')
        
        # 添加数据标签
        def add_labels(bars):
            for bar in bars:
                height = bar.get_height()
                ax1.text(bar.get_x() + bar.get_width()/2., height + 1,
                        f'{height:.2f}', ha='center', va='bottom', fontsize=9)
        
        add_labels(bars1)
        add_labels(bars2)
        
        # 设置左Y轴标签
        ax1.set_ylabel('Execution Time (ms)', fontsize=12)
        ax1.set_xticks(x)
        ax1.set_xticklabels(opt_levels)
        ax1.set_xlabel('Compiler Optimization Level', fontsize=12)
        ax1.legend(loc='upper left')
        
        # 右Y轴: 加速比
        ax2 = ax1.twinx()
        ax2.plot(x, speedups, 'r-', marker='o', label='Speedup')
        ax2.set_ylabel('Speedup (x)', fontsize=12, color='r')
        ax2.tick_params(axis='y', colors='r')
        
        # 添加加速比数据标签
        for i, speedup in enumerate(speedups):
            ax2.text(i, speedup + 0.1, f'{speedup:.2f}x', ha='center', va='bottom', color='r', fontsize=9)
        
        ax2.legend(loc='upper right')
        
        # 图表标题
        plt.title('Effect of Compiler Optimization on Matrix-Vector Multiplication', fontsize=14)
        plt.grid(True, alpha=0.3)
        
        # 保存图表
        plt.tight_layout()
        save_path = os.path.join(output_dir, 'compiler_opt_matrix.png')
        plt.savefig(save_path)
        plt.close()
        
        print(f"矩阵-向量乘法编译器优化图表已生成: {save_path}")
        
    except FileNotFoundError:
        print("警告: 找不到编译器优化的矩阵乘法数据文件 results/compiler_opt_matrix.csv")
        # 生成一个带有错误信息的简单图表
        plt.figure(figsize=(8, 6))
        ax = plt.gca()
        ax.text(0.5, 0.5, "数据文件未找到\n请先运行编译器优化实验",
                horizontalalignment='center', verticalalignment='center',
                fontsize=14, transform=ax.transAxes)
        ax.axis('off')
        save_path = os.path.join(output_dir, 'compiler_opt_matrix.png')
        plt.savefig(save_path)
        plt.close()
    except Exception as e:
        print(f"生成矩阵-向量乘法编译器优化图表时出错: {e}")

def plot_sum_array_optimization():
    """绘制编译器优化级别对数组求和的影响"""
    try:
        # 读取数据
        df = pd.read_csv('results/compiler_opt_sum.csv')
        
        # 创建图表
        ax = set_figure_style()
        
        # 准备绘图数据
        opt_levels = df['Optimization Level'].tolist()
        naive_times = df['Naive Algorithm'].astype(float).tolist()
        dual_times = df['Dual Path Algorithm'].astype(float).tolist()
        recursive_times = df['Recursive Algorithm'].astype(float).tolist()
        dual_speedups = df['Dual Path Speedup'].astype(float).tolist()
        recursive_speedups = df['Recursive Speedup'].astype(float).tolist()
        
        # 创建主图表
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), dpi=300, gridspec_kw={'height_ratios': [2, 1]})
        
        # 上部图表: 执行时间
        width = 0.25
        x = np.arange(len(opt_levels))
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
        
        # 设置上部图表标签
        ax1.set_ylabel('Execution Time (ms)', fontsize=12)
        ax1.set_xticks(x)
        ax1.set_xticklabels(opt_levels)
        ax1.set_title('Execution Time by Optimization Level', fontsize=14)
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # 下部图表: 加速比
        width = 0.35
        x = np.arange(len(opt_levels))
        bars4 = ax2.bar(x - width/2, dual_speedups, width, label='Dual Path Speedup', color='#ff7f0e')
        bars5 = ax2.bar(x + width/2, recursive_speedups, width, label='Recursive Speedup', color='#2ca02c')
        
        # 添加数据标签
        add_labels(bars4)
        add_labels(bars5)
        
        # 设置下部图表标签
        ax2.set_ylabel('Speedup (x)', fontsize=12)
        ax2.set_xticks(x)
        ax2.set_xticklabels(opt_levels)
        ax2.set_xlabel('Compiler Optimization Level', fontsize=12)
        ax2.set_title('Speedup by Optimization Level', fontsize=14)
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        # 整体标题
        plt.suptitle('Effect of Compiler Optimization on Array Summation', fontsize=16)
        
        # 保存图表
        plt.tight_layout(rect=[0, 0, 1, 0.96])  # 为整体标题留出空间
        save_path = os.path.join(output_dir, 'compiler_opt_sum.png')
        plt.savefig(save_path)
        plt.close()
        
        print(f"数组求和编译器优化图表已生成: {save_path}")
        
    except FileNotFoundError:
        print("警告: 找不到编译器优化的数组求和数据文件 results/compiler_opt_sum.csv")
        # 生成一个带有错误信息的简单图表
        plt.figure(figsize=(8, 6))
        ax = plt.gca()
        ax.text(0.5, 0.5, "数据文件未找到\n请先运行编译器优化实验",
                horizontalalignment='center', verticalalignment='center',
                fontsize=14, transform=ax.transAxes)
        ax.axis('off')
        save_path = os.path.join(output_dir, 'compiler_opt_sum.png')
        plt.savefig(save_path)
        plt.close()
    except Exception as e:
        print(f"生成数组求和编译器优化图表时出错: {e}")

def main():
    plot_matrix_vector_optimization()
    plot_sum_array_optimization()
    print(f"编译器优化效果图表生成完成，保存在 {output_dir}/ 目录")

if __name__ == "__main__":
    main() 