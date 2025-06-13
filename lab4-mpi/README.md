# MPI并行数论变换(NTT)实现

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![MPI](https://img.shields.io/badge/MPI-OpenMPI-green.svg)](https://www.open-mpi.org/)

基于消息传递接口(MPI)的高性能并行数论变换(Number Theoretic Transform, NTT)算法实现，集成Barrett模约简优化技术，适用于大规模多项式乘法和密码学计算。

## 项目概述

数论变换(NTT)是快速傅里叶变换(FFT)在有限域上的扩展，在现代密码学、同态加密和高效多项式乘法等领域具有重要应用。本项目实现了基于MPI的并行NTT算法，通过Barrett模约简等优化技术显著提升计算效率。

### 主要特性

- 🚀 **高性能并行计算**: 基于MPI实现的并行NTT算法，最高加速比达2.22倍
- 🔧 **Barrett模约简优化**: 采用高效的模运算优化，避免除法运算瓶颈
- 🌐 **跨平台兼容**: 支持x86和ARM架构，在多种服务器环境下验证
- 📊 **全面性能测试**: 涵盖多种模数、数据规模和进程配置的系统性测试
- 🎯 **精确结果**: 避免浮点数精度问题，确保计算结果的准确性

## 技术亮点

### 算法优化
- **Barrett模约简**: 预计算优化常数，将除法转换为乘法和位移操作
- **主从并行模式**: 结合点值乘法并行化，平衡计算效率和通信开销
- **内存访问优化**: 优化数据布局，提高缓存命中率

### 性能表现
- 在数据规模10000时，2进程配置实现2.12倍加速比
- 在数据规模1000时，最高达到2.22倍加速比
- 支持1000、10000、100000等多种数据规模
- 测试覆盖1、2、4、8个进程配置

## 目录结构

```
.
├── src/                    # 源代码目录
│   ├── common_mpi_ntt.h   # MPI NTT核心头文件
│   ├── common_mpi_ntt.cpp # MPI NTT核心实现
│   ├── ntt_mpi.cpp        # 主程序入口
│   ├── Makefile           # 编译配置
│   └── *.sh               # 测试脚本
├── src-arm/               # ARM平台源码
├── fig/                   # 实验图表
│   ├── mpi_performance.png
│   ├── test.png
│   └── NKU.png
├── style/                 # LaTeX样式文件
├── main.tex              # 实验报告
├── reference.bib         # 参考文献
└── README.md             # 项目说明
```

## 环境要求

### 系统环境
- **操作系统**: Linux (推荐Ubuntu 20.04+)
- **编译器**: g++ (支持C++17标准)
- **MPI实现**: OpenMPI 4.0+
- **架构支持**: x86_64, ARM64

### 软件依赖
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential
sudo apt install openmpi-bin openmpi-common libopenmpi-dev

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install openmpi openmpi-devel
```

## 编译安装

### 1. 克隆仓库
```bash
git clone https://github.com/aokimi0/parallel-programming.git
cd parallel-programming
```

### 2. 编译程序
```bash
cd src
make
```

### 3. 验证安装
```bash
# 生成测试数据
./generate_test_data

# 运行单进程测试
mpirun -np 1 ./ntt_mpi < test_input.txt

# 运行多进程测试
mpirun -np 2 ./ntt_mpi < test_input.txt
```

## 使用方法

### 基本用法

```bash
# 运行MPI并行NTT
mpirun -np <进程数> ./ntt_mpi < input_file.txt
```

### 输入格式
```
<多项式系数个数> <模数>
<多项式A的系数>
<多项式B的系数>
```

### 示例
```bash
# 创建输入文件
echo "4 7340033" > input.txt
echo "1 2 3 4" >> input.txt
echo "5 6 7 8" >> input.txt

# 运行2进程并行计算
mpirun -np 2 ./ntt_mpi < input.txt
```

### 性能测试

```bash
# 运行完整性能测试
cd src
./test_performance.sh

# 运行正确性测试
./correctness_test.sh
```

## 性能基准

### 测试环境
- **平台**: WSL Ubuntu 24.04 (x86_64)
- **处理器**: Intel/AMD 多核处理器
- **MPI**: OpenMPI 4.1.x
- **编译器**: g++ 11.x

### 性能数据

| 模数 | 数据规模 | 1进程(μs) | 2进程(μs) | 4进程(μs) | 8进程(μs) | 最佳加速比 |
|------|----------|-----------|-----------|-----------|-----------|------------|
| 7340033 | 1000 | 119.60 | 599.44 | 176.35 | 1082.27 | 0.68 |
| 7340033 | 10000 | 10764.4 | 5089.28 | 5723.23 | 7585.11 | **2.12** |
| 7340033 | 100000 | 50611.3 | 36636.7 | 49637.8 | 68674.0 | **1.38** |
| 104857601 | 1000 | 316.33 | 142.31 | 196.10 | 5842.52 | **2.22** |

## 算法原理

### 数论变换基础
NTT基于模域中的原根性质，将多项式乘法转换为点值乘法：

```
前向变换: Y_k = Σ(j=0 to n-1) X_j * ω_n^(jk) mod p
逆向变换: X_j = n^(-1) * Σ(k=0 to n-1) Y_k * ω_n^(-jk) mod p
```

### Barrett模约简
优化模运算，避免除法操作：
```
x mod q ≈ x - ⌊xr/2^k⌋ * q
其中 r = ⌊2^k/q⌋ 为预计算常数
```

### 并行策略

1. **主从NTT计算**: 主进程执行NTT变换，广播结果
2. **点值乘法并行**: 各进程并行处理点值乘法，聚合结果

## 应用场景

- **密码学**: RSA、椭圆曲线密码学中的大整数运算
- **同态加密**: 隐私保护计算中的多项式运算
- **信号处理**: 数字滤波器设计和频域分析
- **计算数学**: 高精度数值计算和符号计算

## 实验报告

详细的性能分析和技术报告请参见 [实验报告](lab4_mpi.pdf)

## 相关项目

[lab3-pthread](lab3-pthread/) - 多线程NTT实现参考 