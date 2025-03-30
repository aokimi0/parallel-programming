#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Arrow
import matplotlib.colors as mcolors
import os

# 创建图片目录
if not os.path.exists('fig'):
    os.makedirs('fig')

def plot_access_pattern_diagram():
    # 创建一个2行1列的子图布局
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 8))
    
    # 定义矩阵尺寸和存储布局
    n_rows, n_cols = 5, 5
    
    # 颜色设置
    memory_color = 'lightgray'
    grid_color = 'black'
    arrow_color1 = 'darkred'
    arrow_color2 = 'darkblue'
    text_color = 'black'
    
    # 1. 上图：列访问模式
    ax1.set_title('Column-Major Access Pattern', fontsize=14)
    
    # 绘制内存布局（行主序）
    for i in range(n_rows):
        for j in range(n_cols):
            rect = Rectangle((j, n_rows-1-i), 0.9, 0.9, facecolor=memory_color, edgecolor=grid_color, alpha=0.5)
            ax1.add_patch(rect)
            ax1.text(j+0.45, n_rows-1-i+0.45, f'A[{i}][{j}]', ha='center', va='center', fontsize=10, color=text_color)
    
    # 绘制访问顺序（列主序）
    arrows = []
    arrow_positions = []
    
    # 第一列的访问顺序
    for i in range(n_rows-1):
        arrow_positions.append(((0.45, n_rows-1-i-0.1), (0, -0.8)))
    
    # 从第一列末尾到第二列开头
    arrow_positions.append(((0.45, 0.45), (1, 0)))
    
    # 第二列的访问顺序
    for i in range(n_rows-1):
        arrow_positions.append(((1.45, n_rows-1-i-0.1), (0, -0.8)))
    
    # 添加一些箭头指示后续列
    arrow_positions.append(((1.45, 0.45), (1, 0)))
    
    # 绘制所有箭头
    for i, ((x, y), (dx, dy)) in enumerate(arrow_positions):
        if i == n_rows-1 or i == 2*n_rows:  # 横向箭头
            color = arrow_color2
            width = 0.05
            head_width = 0.15
        else:  # 纵向箭头
            color = arrow_color1
            width = 0.05
            head_width = 0.15
            
        ax1.arrow(x, y, dx, dy, head_width=head_width, head_length=0.15, 
                 fc=color, ec=color, width=width, length_includes_head=True, alpha=0.8)
    
    # 添加标注
    ax1.annotate('Column-by-column access\n(Memory access stride = n)', 
                xy=(3, 2), xytext=(4, 3),
                ha='center', va='center',
                bbox=dict(boxstyle="round,pad=0.3", fc='white', ec='gray', alpha=0.8),
                arrowprops=dict(arrowstyle="->", connectionstyle="arc3,rad=.2", color='gray'),
                fontsize=10)
    
    ax1.annotate('Row-major memory layout', 
                xy=(2, -0.5), xytext=(2, -0.5),
                ha='center', va='center', fontsize=12, color=text_color)
    
    # 2. 下图：行访问模式
    ax2.set_title('Row-Major Access Pattern', fontsize=14)
    
    # 绘制内存布局（行主序）- 与上图相同
    for i in range(n_rows):
        for j in range(n_cols):
            rect = Rectangle((j, n_rows-1-i), 0.9, 0.9, facecolor=memory_color, edgecolor=grid_color, alpha=0.5)
            ax2.add_patch(rect)
            ax2.text(j+0.45, n_rows-1-i+0.45, f'A[{i}][{j}]', ha='center', va='center', fontsize=10, color=text_color)
    
    # 绘制访问顺序（行主序）
    arrows = []
    arrow_positions = []
    
    # 第一行的访问顺序
    for i in range(n_cols-1):
        arrow_positions.append(((i+0.55, n_rows-1-0+0.45), (0.9, 0)))
    
    # 从第一行末尾到第二行开头
    arrow_positions.append(((n_cols-1+0.45, n_rows-1-0+0.45), (0, -1)))
    
    # 第二行的访问顺序
    for i in range(n_cols-1):
        arrow_positions.append(((i+0.55, n_rows-1-1+0.45), (0.9, 0)))
    
    # 添加一些箭头指示后续行
    arrow_positions.append(((n_cols-1+0.45, n_rows-1-1+0.45), (0, -1)))
    
    # 绘制所有箭头
    for i, ((x, y), (dx, dy)) in enumerate(arrow_positions):
        if i == n_cols-1 or i == 2*n_cols:  # 纵向箭头
            color = arrow_color2
            width = 0.05
            head_width = 0.15
        else:  # 横向箭头
            color = arrow_color1
            width = 0.05
            head_width = 0.15
            
        ax2.arrow(x, y, dx, dy, head_width=head_width, head_length=0.15, 
                 fc=color, ec=color, width=width, length_includes_head=True, alpha=0.8)
    
    # 添加标注
    ax2.annotate('Row-by-row access\n(Memory access stride = 1)', 
                xy=(3, 3), xytext=(4, 1.5),
                ha='center', va='center',
                bbox=dict(boxstyle="round,pad=0.3", fc='white', ec='gray', alpha=0.8),
                arrowprops=dict(arrowstyle="->", connectionstyle="arc3,rad=.2", color='gray'),
                fontsize=10)
    
    ax2.annotate('Row-major memory layout', 
                xy=(2, -0.5), xytext=(2, -0.5),
                ha='center', va='center', fontsize=12, color=text_color)
    
    # 共同设置
    for ax in [ax1, ax2]:
        ax.set_xlim(-0.5, n_cols + 0.5)
        ax.set_ylim(-1, n_rows + 0.5)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_aspect('equal')
    
    # 添加缓存未命中率和性能对比信息
    fig.text(0.5, 0.01, "Figure 2: Matrix Memory Access Patterns and Their Performance Impact", 
             ha='center', fontsize=12, weight='bold')
    
    # 添加效果说明
    fig.text(0.97, 0.6, "• High cache miss rate (≈33.2%)\n• Poor spatial locality\n• Performance: 123.36ms (4000×4000)", 
             va='center', ha='right', fontsize=10,
             bbox=dict(boxstyle="round,pad=0.3", fc='mistyrose', ec='lightcoral', alpha=0.8))
    
    fig.text(0.97, 0.25, "• Low cache miss rate (≈3.3%)\n• Excellent spatial locality\n• Performance: 17.09ms (4000×4000)", 
             va='center', ha='right', fontsize=10,
             bbox=dict(boxstyle="round,pad=0.3", fc='honeydew', ec='lightgreen', alpha=0.8))
    
    plt.tight_layout()
    plt.subplots_adjust(top=0.95, bottom=0.05)
    plt.savefig('fig/access_patterns_diagram.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Access patterns diagram saved as fig/access_patterns_diagram.png")

if __name__ == "__main__":
    plot_access_pattern_diagram() 