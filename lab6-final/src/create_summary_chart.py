import matplotlib.pyplot as plt
import numpy as np
import os

def create_summary_chart():
    labels = [
        '串行 (基准)', 
        'SIMD (AVX2)', 
        'Pthread (8线程)', 
        'MPI (2进程)', 
        'GPU (基础)', 
        'GPU (Montgomery)'
    ]
    speedups = [
        1.0,   
        1.35,  
        4.5,   
        2.2,   
        33.6,  
        78.0   
    ]
    
    colors = [
        '#8c8c8c',  
        '#ff7f0e',  
        '#1f77b4',  
        '#2ca02c',  
        '#d62728',  
        '#9467bd'   
    ]

    try:
        plt.rcParams['font.sans-serif'] = [
            'SimHei', 'Noto Sans CJK SC', 'WenQuanYi Zen Hei', 'Source Han Sans CN', 'DejaVu Sans'
        ]
        plt.rcParams['axes.unicode_minus'] = False
    except Exception as e:
        print(f"Could not set Chinese font, will proceed with default. Error: {e}")

    fig, ax = plt.subplots(figsize=(12, 7))

    bars = ax.barh(labels, speedups, color=colors)

    ax.set_xlabel('相对于CPU串行版本的加速比 (Speedup)', fontsize=14)
    ax.set_title('不同并行优化策略的NTT性能对比', fontsize=18, pad=20)
    ax.invert_yaxis()  
    ax.xaxis.grid(True, linestyle='--', which='major', color='grey', alpha=.5)
    ax.set_xscale('log') 
    
    ax.set_xticks([1, 2, 5, 10, 20, 50, 100])
    ax.get_xaxis().set_major_formatter(plt.ScalarFormatter())

    for bar in bars:
        width = bar.get_width()
        label_x_pos = width * 1.1
        ax.text(label_x_pos, bar.get_y() + bar.get_height()/2, f'{width:.1f}x', 
                va='center', ha='left', fontsize=12)
        
    plt.xlim(right=max(speedups) * 1.5) 
    plt.tight_layout()

    output_dir = '../fig'
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    output_path = os.path.join(output_dir, 'summary_performance_comparison.png')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    
    print(f"Chart saved to {os.path.abspath(output_path)}")

if __name__ == '__main__':
    create_summary_chart() 