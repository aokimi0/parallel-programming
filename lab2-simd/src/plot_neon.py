import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# 读取数据
df = pd.read_csv('src/neon_data.csv', encoding='utf-8')

# 确保fig/目录存在
os.makedirs('fig', exist_ok=True)

# 1. 绘制大规模数据下不同模数的性能对比
# 过滤大规模数据
big_df = df[df['size'] == 131072]

# 按实现方式分组
plt.figure(figsize=(10, 6))
implementations = big_df['implementation'].unique()
mod_labels = ['7340033', '104857601', '469762049', '1337006139375617']

# 过滤mod_labels中可能不存在的值
mod_labels = [mod for mod in mod_labels if big_df[big_df['modulus'] == int(mod)].shape[0] > 0]

ind = np.arange(len(mod_labels))
width = 0.2
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']

impl_map = {
    '朴素NTT': 'Naive NTT',
    'NEON优化': 'NEON Optimized',
    'DIF优化': 'DIF Optimized',
    'NEON+DIF优化': 'NEON+DIF'
}

for i, impl in enumerate(implementations):
    impl_data = big_df[big_df['implementation'] == impl]
    times = []
    
    for mod in mod_labels:
        mod_data = impl_data[impl_data['modulus'] == int(mod)]
        if len(mod_data) > 0:
            times.append(mod_data['time_us'].values[0])
        else:
            times.append(0)  # 如果没有对应数据，用0填充
    
    plt.bar(ind + i*width, times, width, label=impl_map[impl], color=colors[i])

plt.ylabel('Time (us)')
plt.xlabel('Modulus')
plt.title('NEON Platform NTT Performance by Modulus (n=131072)')
plt.xticks(ind + width*1.5, mod_labels)
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig('fig/neon_modulus_comparison.png')
plt.close()

# 2. 绘制NEON优化和DIF优化对比图
# 只选择NEON和DIF优化
compare_df = big_df[big_df['implementation'].isin(['NEON优化', 'DIF优化', 'NEON+DIF优化'])]

# 按实现方式分组
plt.figure(figsize=(10, 6))
implementations = ['NEON优化', 'DIF优化', 'NEON+DIF优化']

ind = np.arange(len(mod_labels))
width = 0.25

for i, impl in enumerate(implementations):
    impl_data = compare_df[compare_df['implementation'] == impl]
    times = []
    
    for mod in mod_labels:
        mod_data = impl_data[impl_data['modulus'] == int(mod)]
        if len(mod_data) > 0:
            times.append(mod_data['time_us'].values[0])
        else:
            times.append(0)
    
    plt.bar(ind + i*width, times, width, label=impl_map[impl], color=colors[i+1])

plt.ylabel('Time (us)')
plt.xlabel('Modulus')
plt.title('NEON Platform Optimization Comparison (n=131072)')
plt.xticks(ind + width, mod_labels)
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig('fig/neon_optimization_comparison.png')
plt.close()

# 3. 加速比对比图
plt.figure(figsize=(10, 6))
baseline = big_df[big_df['implementation'] == '朴素NTT']

for i, impl in enumerate(['NEON优化', 'DIF优化', 'NEON+DIF优化']):
    impl_data = big_df[big_df['implementation'] == impl]
    speedups = []
    
    for mod in mod_labels:
        baseline_time = baseline[baseline['modulus'] == int(mod)]['time_us'].values[0]
        impl_time = impl_data[impl_data['modulus'] == int(mod)]['time_us'].values[0]
        speedup = baseline_time / impl_time
        speedups.append(speedup)
    
    plt.bar(ind + i*width, speedups, width, label=impl_map[impl], color=colors[i+1])

plt.ylabel('Speedup')
plt.xlabel('Modulus')
plt.title('NEON Platform Speedup Comparison (n=131072)')
plt.xticks(ind + width, mod_labels)
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()
plt.savefig('fig/neon_speedup_comparison.png')
plt.close()

print('Generated NEON Charts:')
print('1. fig/neon_modulus_comparison.png - Performance by modulus')
print('2. fig/neon_optimization_comparison.png - Optimization comparison')
print('3. fig/neon_speedup_comparison.png - Speedup comparison') 