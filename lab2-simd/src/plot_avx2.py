import pandas as pd
import matplotlib.pyplot as plt
import os

# 读取数据
df = pd.read_csv('src/avx2_benchmark.csv', encoding='utf-8')

# 预处理
# 提取实现名
df['impl'] = df['filename'].apply(lambda x: x.split('/')[-1].replace('.cpp',''))
# 提取规模（用行顺序区分，或可手动指定）
size_labels = ['n=1000', 'n=10000', 'n=100000']
# 按模数分组
mod_list = df['mod'].unique()
impl_list = ['ntt', 'dif', 'avx2', 'dif_avx2']

# 画图
def plot_for_mod(mod, save_path):
    plt.figure(figsize=(8,5))
    for impl in impl_list:
        sub = df[(df['mod']==mod) & (df['impl']==impl)]
        if len(sub) == 0:
            continue
        # 提取时间
        y = sub['avg_sec'].str.replace(' 秒','').astype(float).values
        plt.plot(size_labels, y, marker='o', label=impl)
    plt.title(f'AVX2 NTT Performance (mod={mod})')
    plt.xlabel('Input Size')
    plt.ylabel('Time (s)')
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()

# 确保fig/目录存在
os.makedirs('fig', exist_ok=True)

# 依次画每个模数的图
to_report = []
for mod in mod_list:
    save_path = f'fig/avx2_{mod}.png'
    plot_for_mod(mod, save_path)
    to_report.append((mod, save_path))

# 输出生成的图片路径，便于在report.md中引用
print('生成的图表：')
for mod, path in to_report:
    print(f'mod={mod}: {path}') 