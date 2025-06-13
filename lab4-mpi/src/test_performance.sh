#!/bin/bash

# 确保在正确的目录中
cd "$(dirname "$0")"

# 确保fig目录存在
mkdir -p ../fig

make clean
make all

MODS=(7340033 104857601 469762049)
SIZES=(1000 10000 100000)
PROCESSES=(1 2 4 8)

echo "Generating test data..."
for mod in "${MODS[@]}"; do
    for size in "${SIZES[@]}"; do
        ./generate_test_data $size $mod "test_data_${size}_${mod}.txt"
    done
done

echo "Running performance tests..."
echo "Size,Mod,Processes,Time(us)" > performance_results.csv

for mod in "${MODS[@]}"; do
    for size in "${SIZES[@]}"; do
        for proc in "${PROCESSES[@]}"; do
            echo "Testing: size=$size, mod=$mod, processes=$proc"
            
            time_us=$(mpiexec -n $proc ./ntt_mpi < "test_data_${size}_${mod}.txt" 2>&1 | grep "Average time" | awk -F': ' '{print $3}' | awk '{print $1}')
            
            if [ ! -z "$time_us" ]; then
                echo "$size,$mod,$proc,$time_us" >> performance_results.csv
            fi
        done
    done
done

echo "Performance test completed. Results saved to performance_results.csv"

python3 << 'EOF'
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['font.sans-serif'] = [
    'SimHei', 
    'Noto Sans CJK SC', 
    'WenQuanYi Zen Hei',
    'Source Han Sans CN',
    'DejaVu Sans'
]
plt.rcParams['axes.unicode_minus'] = False

try:
    df = pd.read_csv('performance_results.csv')
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    
    for i, mod in enumerate([7340033, 104857601, 469762049]):
        mod_data = df[df['Mod'] == mod]
        
        ax = axes[i//2, i%2]
        
        for proc in [1, 2, 4, 8]:
            proc_data = mod_data[mod_data['Processes'] == proc]
            if not proc_data.empty:
                ax.loglog(proc_data['Size'], proc_data['Time(us)'], 
                         'o-', label=f'{proc} 进程', linewidth=2)
        
        ax.set_xlabel('多项式大小')
        ax.set_ylabel('执行时间 (微秒)')
        ax.set_title(f'模数 {mod} 的性能表现')
        ax.legend()
        ax.grid(True, alpha=0.3)
    
    speedup_data = df.pivot_table(values='Time(us)', index=['Size', 'Mod'], columns='Processes')
    
    ax = axes[1, 1]
    for mod in [7340033, 104857601, 469762049]:
        mod_speedup = []
        sizes = []
        for size in [1000, 10000, 100000]:
            if (size, mod) in speedup_data.index:
                baseline = speedup_data.loc[(size, mod), 1]
                speedup_8 = baseline / speedup_data.loc[(size, mod), 8] if 8 in speedup_data.columns else None
                if speedup_8 and not np.isnan(speedup_8):
                    mod_speedup.append(speedup_8)
                    sizes.append(size)
        
        if mod_speedup:
            ax.semilogx(sizes, mod_speedup, 'o-', label=f'模数 {mod}', linewidth=2)
    
    ax.axhline(y=8, color='r', linestyle='--', alpha=0.7, label='理想加速比 (8x)')
    ax.set_xlabel('多项式大小')
    ax.set_ylabel('加速比 (相对于单进程)')
    ax.set_title('8进程相对于单进程的加速比')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('../fig/mpi_performance.png', dpi=300, bbox_inches='tight')
    print("性能图表已保存到 ../fig/mpi_performance.png")
    
except Exception as e:
    print(f"生成图表时出错: {e}")
EOF

echo "Cleaning up test data..."
rm -f test_data_*.txt 