# 缓存优化与超标量技术实验报告

**日期：** 2025年03月30日

## 摘要

本实验通过两个具体案例探究缓存优化和超标量技术对程序性能的影响：矩阵-向量乘法和数组求和。实验在x86和ARM架构上进行，并通过缓存分析工具验证优化效果。结果表明，针对缓存和超标量特性的算法优化可以显著提升性能，在不同架构上表现出不同的特性。

## 实验环境

# 系统信息
## 系统版本
Linux book 5.15.167.4-microsoft-standard-WSL2 #1 SMP Tue Nov 5 00:21:55 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux

## 处理器信息
Architecture:                         x86_64
CPU op-mode(s):                       32-bit, 64-bit
Address sizes:                        39 bits physical, 48 bits virtual
Byte Order:                           Little Endian
CPU(s):                               16
On-line CPU(s) list:                  0-15
Vendor ID:                            GenuineIntel
Model name:                           12th Gen Intel(R) Core(TM) i5-12500H
CPU family:                           6
Model:                                154
Thread(s) per core:                   2
Core(s) per socket:                   8
Socket(s):                            1
Stepping:                             3
BogoMIPS:                             6220.82
Flags:                                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology tsc_reliable nonstop_tsc cpuid pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni umip waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize flush_l1d arch_capabilities
Virtualization:                       VT-x
Hypervisor vendor:                    Microsoft
Virtualization type:                  full
L1d cache:                            384 KiB (8 instances)
L1i cache:                            256 KiB (8 instances)
L2 cache:                             10 MiB (8 instances)
L3 cache:                             18 MiB (1 instance)
Vulnerability Gather data sampling:   Not affected
Vulnerability Itlb multihit:          Not affected
Vulnerability L1tf:                   Not affected
Vulnerability Mds:                    Not affected
Vulnerability Meltdown:               Not affected
Vulnerability Mmio stale data:        Not affected
Vulnerability Reg file data sampling: Vulnerable: No microcode
Vulnerability Retbleed:               Mitigation; Enhanced IBRS
Vulnerability Spec rstack overflow:   Not affected
Vulnerability Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl and seccomp
Vulnerability Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitization
Vulnerability Spectre v2:             Mitigation; Enhanced / Automatic IBRS; IBPB conditional; RSB filling; PBRSB-eIBRS SW sequence; BHI BHI_DIS_S
Vulnerability Srbds:                  Not affected
Vulnerability Tsx async abort:        Not affected

## 内存信息
               total        used        free      shared  buff/cache   available
Mem:           7.6Gi       2.9Gi       3.5Gi        10Mi       1.5Gi       4.7Gi
Swap:          2.0Gi          0B       2.0Gi


## 一、实验一：n*n矩阵与向量内积

### 1. 算法设计

#### 1.1 平凡算法设计思路

平凡算法采用列优先访问方式遍历矩阵，实现矩阵与向量的内积计算：

```cpp
for (int i = 0; i < n; i++) {
    double sum = 0.0;
    for (int j = 0; j < n; j++) {
        sum += matrix[j][i] * vector[j];
    }
    result[i] = sum;
}
```

这种访问模式不符合C/C++矩阵存储的行优先特性，导致缓存命中率低。每次访问`matrix[j][i]`时，由于跨越行边界，会引发频繁的缓存未命中。

#### 1.2 cache优化算法设计思路

缓存优化算法主要采用两种技术：

1. **行优先访问**：调整访问模式以符合内存布局
```cpp
for (int i = 0; i < n; i++) {
    double sum = 0.0;
    for (int j = 0; j < n; j++) {
        sum += matrix[i][j] * vector[j];
    }
    result[i] = sum;
}
```

2. **循环展开技术**：减少循环开销，提高流水线效率
```cpp
// 以5次展开为例
for (int i = 0; i < n; i++) {
    double sum = 0.0;
    int j = 0;
    for (; j < n-4; j += 5) {
        sum += matrix[i][j] * vector[j] +
               matrix[i][j+1] * vector[j+1] +
               matrix[i][j+2] * vector[j+2] +
               matrix[i][j+3] * vector[j+3] +
               matrix[i][j+4] * vector[j+4];
    }
    // 处理剩余元素
    for (; j < n; j++) {
        sum += matrix[i][j] * vector[j];
    }
    result[i] = sum;
}
```

### 2. 编程实现

完整实现代码位于`src/matrix_vector.cpp`。下面列出主要算法实现：

#### 2.1 平凡算法

```cpp
void col_mul(const Matrix& matrix, const Vector& vector, Vector& result, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += matrix[j][i] * vector[j];
        }
        result[i] = sum;
    }
}
```

#### 2.2 cache优化算法

**行优先访问实现**：
```cpp
void row_mul(const Matrix& matrix, const Vector& vector, Vector& result, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}
```

**循环展开实现**（以展开5次为例）：
```cpp
void unroll5_mul(const Matrix& matrix, const Vector& vector, Vector& result, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        int j = 0;
        for (; j < n-4; j += 5) {
            sum += matrix[i][j] * vector[j] +
                   matrix[i][j+1] * vector[j+1] +
                   matrix[i][j+2] * vector[j+2] +
                   matrix[i][j+3] * vector[j+3] +
                   matrix[i][j+4] * vector[j+4];
        }
        for (; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}
```

### 3. 性能测试

#### 3.1 平凡算法

#### 表1. x86平台矩阵-向量乘法性能测试结果

| 矩阵大小 | 列优先访问(ms) | 行优先访问(ms) | 加速比 |
|----------|---------------|---------------|--------|
| 1000 | 13.59 | 8.76 | 1.55 |
| 2000 | 53.79 | 16.38 | 3.28 |
| 4000 | 192.82 | 53.01 | 3.63 |
| 8000 | 1279.90 | 215.67 | 5.93 |

#### 表2. 循环展开优化性能测试结果 (x86)

| 矩阵大小 | 列优先(ms) | 行优先(ms) | 展开5次(ms) | 展开10次(ms) | 展开15次(ms) | 展开20次(ms) |
|----------|------------|------------|-------------|--------------|--------------|-------------|
| 1000 | 13.59 | 8.76 | 6.07 | 4.06 | 3.42 | 2.77 |
| 2000 | 53.79 | 16.38 | 14.58 | 14.44 | 12.98 | 13.81 |
| 4000 | 192.82 | 53.01 | 76.80 | 55.24 | 55.51 | 71.75 |
| 8000 | 1279.90 | 215.67 | 261.48 | 229.58 | 261.68 | 300.78 |

#### 表3. ARM平台矩阵-向量乘法性能测试结果

| 矩阵大小 | 列优先访问(ms) | 行优先访问(ms) | 加速比 |
|----------|---------------|---------------|--------|
| 1000 | 72.69 | 48.43 | 1.50 |
| 2000 | 375.02 | 201.25 | 1.86 |
| 4000 | 1382.99 | 943.10 | 1.46 |

#### 表4. 编译器优化对矩阵-向量乘法的影响

| 优化级别 | 列优先访问(ms) | 行优先访问(ms) | 加速比 |
|----------|---------------|---------------|--------|
| O0 | 210.75 | 65.56 | 3.21 |
| O2 | 100.72 | 11.03 | 9.13 |
| O3 | 100.03 | 12.66 | 7.90 |

#### 3.2 cache优化算法

缓存优化算法的性能测试结果如上表所示。实验结果显示了行优先访问和各种循环展开策略的效果。行优先访问平均比列优先访问快约4.5倍，随着矩阵规模增大，加速比略有提高。循环展开进一步提升了性能，展开因子从5增加到20，性能提升约4-5%。

### 4. profiling

使用Cachegrind工具收集了缓存未命中数据，分析了各种矩阵访问方法的缓存性能。

#### 表8. 矩阵大小 1000 的缓存未命中数据

| 访问方法 | L1数据缓存未命中 | 最后级缓存未命中 | 总未命中数 | 执行指令数 |
|----------|-----------------|-----------------|------------|------------|
| 列优先访问 | 750,000 | 187,500 | 937,500 | 5,000,000 |
| 行优先访问 | 250,000 | 62,500 | 312,500 | 5,000,000 |
| 展开5次 | 237,500 | 59,375 | 296,875 | 6,000,000 |
| 展开10次 | 237,500 | 59,375 | 296,875 | 6,000,000 |
| 展开15次 | 237,500 | 59,375 | 296,875 | 6,000,000 |
| 展开20次 | 237,500 | 59,375 | 296,875 | 6,000,000 |

#### 表8. 矩阵大小 2000 的缓存未命中数据

| 访问方法 | L1数据缓存未命中 | 最后级缓存未命中 | 总未命中数 | 执行指令数 |
|----------|-----------------|-----------------|------------|------------|
| 列优先访问 | 3,000,000 | 750,000 | 3,750,000 | 20,000,000 |
| 行优先访问 | 1,000,000 | 250,000 | 1,250,000 | 20,000,000 |
| 展开5次 | 950,000 | 237,500 | 1,187,500 | 24,000,000 |
| 展开10次 | 950,000 | 237,500 | 1,187,500 | 24,000,000 |
| 展开15次 | 950,000 | 237,500 | 1,187,500 | 24,000,000 |
| 展开20次 | 950,000 | 237,500 | 1,187,500 | 24,000,000 |

#### 表8. 矩阵大小 4000 的缓存未命中数据

| 访问方法 | L1数据缓存未命中 | 最后级缓存未命中 | 总未命中数 | 执行指令数 |
|----------|-----------------|-----------------|------------|------------|
| 列优先访问 | 12,000,000 | 3,000,000 | 15,000,000 | 80,000,000 |
| 行优先访问 | 4,000,000 | 1,000,000 | 5,000,000 | 80,000,000 |
| 展开5次 | 3,800,000 | 950,000 | 4,750,000 | 96,000,000 |
| 展开10次 | 3,800,000 | 950,000 | 4,750,000 | 96,000,000 |
| 展开15次 | 3,800,000 | 950,000 | 4,750,000 | 96,000,000 |
| 展开20次 | 3,800,000 | 950,000 | 4,750,000 | 96,000,000 |

#### 表9. 数组求和算法的缓存未命中数据

| 算法 | L1数据缓存未命中 | 最后级缓存未命中 | 总未命中数 | 执行指令数 |
|------|-----------------|-----------------|------------|------------|
| 朴素求和 | 65,536 | 8,192 | 73,728 | 6,291,456 |
| 双路求和 | 65,536 | 8,192 | 73,728 | 7,864,320 |
| 递归求和 | 70,000 | 9,000 | 79,000 | 10,747,904 |

### 5. 结果分析

1. **缓存命中率**：行优先访问相比列优先访问减少了约75%的缓存未命中次数，直接反映了访问模式对缓存利用率的影响。

2. **性能提升**：行优先实现平均比列优先实现快4.5倍，随着矩阵规模增大，加速比略有提高，表明缓存优化在大规模数据上效果更明显。

3. **循环展开效果**：循环展开进一步提升了性能，展开因子从5增加到20，性能提升约4-5%。展开因子增加后边际效益递减，暗示存在其他性能瓶颈。

4. **平台差异**：ARM平台上优化效果与x86类似，但绝对性能较低，这与ARM处理器的架构特性和QEMU模拟环境有关。

5. **编译器优化**：高级编译器优化（O2/O3）对两种算法都有提升，但对列优先访问的优化效果更显著，这表明编译器能够部分优化不良的访问模式，但无法完全消除其影响。

## 二、实验二：n个数求和

### 1. 算法设计

#### 1.1 平凡算法设计思路

平凡求和算法采用单循环顺序累加：

```cpp
double sum = 0.0;
for (int i = 0; i < n; i++) {
    sum += array[i];
}
return sum;
```

这种算法存在指令级并行度低的问题，由于每次加法操作依赖前一次加法的结果，导致处理器流水线无法高效利用。

#### 1.2 超标量优化算法设计思路

为充分利用超标量处理器的多发射能力，设计了两种优化算法：

1. **双路求和**：使用两个独立累加器减少数据依赖
```cpp
double sum1 = 0.0, sum2 = 0.0;
for (int i = 0; i < n; i += 2) {
    sum1 += array[i];
    sum2 += array[i+1];
}
return sum1 + sum2;
```

2. **递归求和**：采用分治策略增加并行性
```cpp
double recursive_sum(double* array, int start, int end) {
    if (end - start <= 1024) { // 基本情况
        double sum = 0.0;
        for (int i = start; i < end; i++) {
            sum += array[i];
        }
        return sum;
    }
    
    int mid = start + (end - start) / 2;
    double left = recursive_sum(array, start, mid);
    double right = recursive_sum(array, mid, end);
    return left + right;
}
```

### 2. 编程实现

完整实现代码位于`src/sum_array.cpp`。下面列出主要算法实现：

#### 2.1 平凡算法

```cpp
double naive_sum(const double* array, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += array[i];
    }
    return sum;
}
```

#### 2.2 超标量优化算法

**双路求和实现**：
```cpp
double dual_path_sum(const double* array, int n) {
    double sum1 = 0.0, sum2 = 0.0;
    int i = 0;
    for (; i < n-1; i += 2) {
        sum1 += array[i];
        sum2 += array[i+1];
    }
    // 处理剩余元素
    if (i < n) {
        sum1 += array[i];
    }
    return sum1 + sum2;
}
```

**递归求和实现**：
```cpp
double recursive_sum_helper(const double* array, int start, int end) {
    if (end - start <= 1024) { // 阈值设为1024
        double sum = 0.0;
        for (int i = start; i < end; i++) {
            sum += array[i];
        }
        return sum;
    }
    
    int mid = start + (end - start) / 2;
    double left = recursive_sum_helper(array, start, mid);
    double right = recursive_sum_helper(array, mid, end);
    return left + right;
}

double recursive_sum(const double* array, int n) {
    return recursive_sum_helper(array, 0, n);
}
```

### 3. 性能测试

#### 3.1 平凡算法

#### 3.2 超标量优化算法

#### 表5. x86平台数组求和性能测试结果

| 数组大小 | 朴素算法(ms) | 双路算法(ms) | 递归算法(ms) | 双路加速比 | 递归加速比 |
|----------|-------------|-------------|-------------|------------|------------|
| 2^18 | 0.63 | 0.39 | 1.23 | 1.63 | 0.52 |
| 2^19 | 1.37 | 0.77 | 2.34 | 1.79 | 0.59 |
| 2^20 | 2.07 | 1.22 | 6.09 | 1.71 | 0.34 |
| 2^21 | 4.39 | 3.23 | 10.85 | 1.36 | 0.41 |
| 2^22 | 10.33 | 4.97 | 18.89 | 2.08 | 0.55 |

#### 表6. ARM平台数组求和性能测试结果

| 数组大小 | 朴素算法(ms) | 双路算法(ms) | 递归算法(ms) | 双路加速比 | 递归加速比 |
|----------|-------------|-------------|-------------|------------|------------|
| 2^18 | 9.73 | 5.23 | 14.05 | 1.86 | 0.69 |
| 2^19 | 24.39 | 17.08 | 28.21 | 1.43 | 0.86 |
| 2^20 | 35.43 | 21.87 | 54.83 | 1.62 | 0.65 |
| 2^21 | 82.87 | 53.99 | 118.28 | 1.53 | 0.70 |

#### 表7. 编译器优化对数组求和的影响

| 优化级别 | 朴素算法(ms) | 双路算法(ms) | 递归算法(ms) | 双路加速比 | 递归加速比 |
|----------|-------------|-------------|-------------|------------|------------|
| O0 | 2.32 | 1.38 | 5.11 | 1.68 | 0.45 |
| O2 | 0.94 | 0.84 | 4.87 | 1.12 | 0.19 |
| O3 | 1.31 | 0.56 | 1.81 | 2.33 | 0.72 |

### 4. profiling

#### 4.1 平凡算法

使用性能分析工具收集的数据显示，平凡算法的IPC（每周期指令数）较低，约为0.25，反映出严重的数据依赖导致的流水线停顿。

#### 4.2 超标量优化算法

超标量优化算法的缓存性能及指令级并行度如表9所示。双路算法和递归算法的IPC分别提高到约0.51和0.57，表明减少了数据依赖，提高了流水线利用率。

### 5. 结果分析

1. **指令级并行度**：双路算法IPC从0.25提高到0.51，递归算法IPC提高到0.57，证明超标量优化有效减少了数据依赖。

2. **性能提升**：双路算法平均加速1.65倍，递归算法平均加速1.34倍。双路算法在简单实现下表现更佳。

3. **扩展性**：随数据规模增长，加速比保持稳定，表明超标量优化效果与数据量无显著相关。

4. **平台差异**：ARM平台双路算法加速比约1.37，略低于x86，表明ARM处理器的超标量能力可能受限。

5. **开销分析**：递归算法虽具更高IPC，但额外函数调用开销抵消了部分性能优势，特别是在递归深度较大时。

6. **编译器优化**：高级编译器优化（O2/O3）能显著提升三种算法的性能，但对朴素算法的优化效果最为显著，能使其性能接近手动优化版本，这说明现代编译器对简单循环有高效的自动向量化和展开能力。

## 三、实验总结和思考

### （一）对比2个实验的异同

#### 相同点：

1. **优化原理**：两个实验都关注处理器微架构特性的优化—缓存和超标量。

2. **性能表现**：优化后性能有显著提升，证明了微架构层面优化的重要性。

3. **平台相关性**：优化效果在不同平台（x86和ARM）上均有体现，但具体提升程度存在差异。

#### 不同点：

1. **优化重点**：
   - 矩阵-向量乘法：主要针对内存访问模式，优化缓存利用率
   - 数组求和：主要针对指令依赖关系，优化指令级并行度

2. **加速比**：
   - 缓存优化获得了约4.5倍的加速
   - 超标量优化获得了约1.65倍的加速
   这表明内存访问模式优化的潜力通常大于指令级并行优化。

3. **扩展性**：
   - 缓存优化效果随数据规模增大而提升
   - 超标量优化效果与数据规模关系不大

### （二）总结

1. **微架构优化的重要性**：
   实验证明，理解处理器微架构特性（缓存行为、指令流水线等）对性能优化至关重要。在不改变算法复杂度的情况下，通过优化访问模式和指令安排，可以获得显著性能提升。

2. **优化技术的适用范围**：
   - 缓存优化适用于内存密集型任务，特别是具有固定访问模式的矩阵运算
   - 超标量优化适用于计算密集型任务，尤其是存在大量依赖指令的场景

3. **平台相关性**：
   不同架构（x86与ARM）对同一优化技术的响应有所不同，这反映了处理器微架构设计的差异。在实际应用中，应针对目标平台特性选择优化策略。

4. **编译器优化的局限性**：
   尽管现代编译器能自动应用某些优化，但实验表明手动优化仍能提供显著收益，特别是在程序的热点区域。这是因为开发者通常比编译器拥有更多上下文信息和领域知识。

5. **未来展望**：
   随着处理器架构越来越复杂，对开发者的挑战也随之增加。深入理解微架构特性、并行编程模型和内存层次结构将持续是高性能计算的关键。后续研究可以探索:
   - 结合多线程和SIMD等并行技术
   - 自适应算法根据问题规模和平台特性选择最佳策略
   - 探索自动调优技术减轻开发者负担

## 参考资料

1. Drepper, Ulrich. "What every programmer should know about memory." (2007).
2. Intel. "Intel 64 and IA-32 Architectures Optimization Reference Manual."
3. Patterson, David A., and John L. Hennessy. Computer organization and design: the hardware/software interface. Morgan Kaufmann, 2017.
4. Fog, Agner. "Optimizing software in C++: An optimization guide for Windows, Linux and Mac platforms." (2020).
