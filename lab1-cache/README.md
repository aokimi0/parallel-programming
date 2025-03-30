# 缓存优化实验报告

本实验探索了矩阵向量乘法和数组求和操作的缓存优化技术。

## 项目结构

- `src/`: 源代码文件
  - `matrix_vector.cpp`: 矩阵向量乘法实现（朴素版本和缓存优化版本）
  - `sum_array.cpp`: 数组求和实现（朴素版本、双路径和递归版本）
  - `generate_plots.py`: 性能图表生成脚本
  - `plot_cache_misses.py`: 缓存缺失数据可视化脚本
- `bin/`: 编译后的可执行文件
  - `matrix_vector`: 矩阵向量乘法程序
  - `sum_array`: 数组求和程序
- `results/`: 实验结果CSV文件
- `cachegrind_logs/`: Cachegrind分析文件
- `fig/`: 图表和图像文件
- `report/`: 报告相关文件
- `style/`: 文档样式文件
- `reference/`: 参考资料
- `arm_build/`: ARM平台编译文件
- `.vscode/`: VSCode配置文件
- 主要文件：
  - `main.tex`: 实验报告LaTeX源文件
  - `reference.bib`: 参考文献配置
  - `run_complete_experiment.sh`: 运行所有实验的脚本
  - `cache_optimization_report.md`: 完整实验报告
  - `cache_optimization_compressed.md`: 压缩版实验报告
  - `requirement.md`: 实验要求说明
  - `lab1_cache.pdf`: 最终PDF报告

## 实验内容

### 1. 矩阵向量乘法优化

比较不同的矩阵向量乘法实现方式：

- 列优先访问（朴素实现，对缓存不友好）
- 行优先访问（缓存友好）
- 循环展开（展开因子分别为5、10、15和20）

### 2. 数组求和优化

比较不同的数组求和实现方式：

- 朴素线性求和
- 双路径求和（减少依赖链）
- 递归分治求和

## 运行实验

运行所有实验并生成图表：

```bash
bash run_complete_experiment.sh
```

该脚本将：
1. 将源代码编译到`bin/`目录
2. 对两种算法进行性能测试
3. 使用Valgrind的Cachegrind工具收集缓存缺失数据
4. 在`fig/`目录生成性能图表
5. 生成实验报告（支持MD和PDF格式）

### 运行单个算法

矩阵向量乘法：

```bash
bin/matrix_vector <矩阵大小> <方法ID>
```

方法ID：
- 0：运行所有方法（默认）
- 1：列优先访问
- 2：行优先访问
- 3-6：循环展开（5、10、15、20）

数组求和：

```bash
bin/sum_array <数组大小>
```

## 环境要求

- WSL2-Ubuntu24.04（x86-64）
- GCC 13.3.0
- Python 3.10+（需要matplotlib、numpy和pandas库）
- Valgrind（用于缓存分析）
- QEMU 7.2.0 (模拟arm64架构) 

## 实验结果

行优先的矩阵访问模式比列优先模式可以获得约4倍的性能提升，主要原因是更好地利用了CPU缓存的空间局部性。数组求和实验中，双路径算法和递归算法也展示了指令级并行和时间局部性对性能的积极影响。

详细结果可查看：

- PDF实验报告：`lab1_cache.pdf`
- 原始实验结果：`report/cache_optimization_report.md`