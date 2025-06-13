# ARM平台MPI NTT测试

## 文件说明

- `common_mpi_ntt.h/cpp` - MPI NTT算法核心实现
- `ntt_mpi_arm.cpp` - ARM平台测试主程序
- `Makefile` - ARM编译配置

## 编译和运行

### 编译
```bash
make
```

### 基本测试
```bash
make test
```

### 性能基准测试
```bash
make benchmark
```

### 清理
```bash
make clean
```

## 测试输出

程序会生成以下文件夹和文件：
- `files/` - 测试数据输出目录
- `files/output_*.txt` - 不同规模的测试结果

## 功能特性

- Barrett模约简优化
- MPI多进程并行化
- 自动性能计时
- 多种问题规模测试
- 结果文件输出 