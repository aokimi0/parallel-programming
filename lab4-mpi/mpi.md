# MPI 编程实验——以高斯消去为例

## 2 多进程与多线程介绍

[cite_start]本次实验涉及到的MPI编程是多进程编程, 什么是进程？本节介绍和线程和进程的区别于联系。首先明确: OpenMP和Pthread均为多线程编程。 [cite: 5]

### 2.1 进程与线程

[cite_start]进程(process)是操作系统进行资源分配的最小单元, 线程(thread)是操作系统进行运算调度的最小单元。 [cite: 5]

[cite_start]通常情况下, 我们在运行程序时, 比如在Linux下通过`./test`运行一个可执行文件, 那么我们就相当于创建了一个进程, 操作系统会为这个进程分配ID 和堆栈空间等资源。 [cite: 5] [cite_start]如果这个可执行程序是个多线程程序(比如由OpenMP或Pthread 编写的cpp文件编译而来), 那么这个`./test`进程在执行到某个特定位置时会创建多个线程继续执行。 [cite: 5] [cite_start]所以, 进程可以包含多个线程, 但每个线程只能属于一个进程。 [cite: 5]

[cite_start][Image: 进程与线程概念图, 展示了3个独立的进程, 每个进程内部可以有1个或多个线程。] [cite: 6]

[cite_start]如图2.1所示, 每个进程有独立的地址空间(外部框表示), 同一进程内不同线程(曲线)共享该进程的内存。 [cite: 6] [cite_start]但是不同进程不共享地址空间, 如果一个进程要访问另一个进程的数据就只能通过特定的函数进行通信。 [cite: 6]

### 2.2 MPI 多进程

[cite_start]区别于普通的可执行程序只创建一个进程, 通过`mpiexec -n num ./test`运行的程序会根据`-n`选项后的参数`num`, 创建`num`个进程运行`test`可执行程序。 [cite: 6]

[cite_start]课程中提到了在开始编写MPI部分代码时调用`MPI_Init`完成MPI初始化, 在MPI部分结束时调用`MPI_Finalize`完成清理工作。 [cite: 6] [cite_start]这两个函数与多线程中创建和销毁线程有本质的区别: 程序并非只在两个函数调用之间的部分并行执行, 而是整体并行执行, 在两个函数之间的部分完成数据划分运算、通信等工作。 [cite: 6] 以下方程序为例, 代码在两个函数调用的外部定义了`size`变量和`rank`变量, 同时输出`Hello World!`; [cite_start]在两个函数之间的部分为`size`和`rank`赋值并输出。 [cite: 6] [cite_start]编译并通过`mpiexec -n num ./test`运行后, 每个进程均会输出一次`Hello World!`, 共输出`num`次, 这也验证并行并非只发生在`MPI_Init`和`MPI_Finalize`之间的代码。 [cite: 6]

```cpp
#include "mpi.h"
#include <iostream>

int main(int argc, char argv[]) {
    int size, rank;
    std::cout << "Hello world!" << std::endl;
    MPI_Init(&argc, &argv);
    size = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();
    std::cout << "MPI size:" << size << " rank:" << rank << std::endl;
    MPI_Finalize();
    return 0;
}
```
[cite_start][cite: 7, 9]

---

### 3.3 作业注意要点及建议

1.  [cite_start]**矩阵数值初始化问题** [cite: 20]
    [cite_start]为了避免计算中出现`inf`或`nan`的问题, 建议初始化矩阵时可以首先初始化一个上三角矩阵, 然后随机的抽取若干行去将它们相加减然后执行若干次, 这样可以保证矩阵不会有`inf`和`nan`。 [cite: 20]
2.  [cite_start]**不同任务划分策略** [cite: 21]
    [cite_start]可结合多线程以及 SIMD 并行进行任务划分。 [cite: 21] 例如, 对于一维行划分, 可看作是将第二层循环拆分, 分配给不同进程; 这样, 继续进行多线程并行, 即可继续对第二层循环进行划分(将进程负责的行划分给内部的多个线程), 也可以对最内层循环进行划分(将进程负责的所有行的不同列分配给不同线程); [cite_start]而再继续结合SIMD 并行化, 则只能对最内层循环进行向量化。 [cite: 21]
3.  [cite_start]**计算误差与程序正确性** [cite: 21]
    [cite_start]建议采取多次测量取均值的方法确定较合理的性能测试结果。 [cite: 21] [cite_start]考虑到实验平台等因素, 计时测试工具可以考虑使用MPI计时。 [cite: 21]

## 5 NTT 选题指导

### 5.1 新的规约优化

[cite_start]目前多数同态加密库里的模数优化并非 Montgomery规约, 而是Barrett规约, 因为其在非SIMD加速条件下加速比更高。 [cite: 22]

[cite_start]Barrett 模乘是一种近似算法。 [cite: 22] [cite_start]取模公式为: $x~mod~q=x-\lfloor xs\rfloor q$, 其中 $s=\frac{1}{q}$。 [cite: 22]
[cite_start]Barrett 模乘使用近似 $\frac{r}{4^{k}}\approx\frac{1}{q}$ (其中 $r=\lfloor\frac{4^{k}}{q}\rfloor$), 可得: [cite: 22]

[cite_start]$x~mod~q=x-\lfloor\frac{xr}{2^{k}}\rfloor q$ [cite: 22]

[cite_start]由于存在误差, 最终输出为 $x~mod~q \in [x~mod~q,(x~mod~q)+q]$。 [cite: 22]

[cite_start]在应用 Barrett 模乘时一般取 $k=32$, $2^{k}=2^{64}$ 以提高近似精度和模数q的最大表示。 [cite: 23] [cite_start]由于`unsigned long long` 的最大值为 $2^{64}-1$, 所以需要转化为`uint128`, 这会占据大量的时间开销。 [cite: 23]

### 5.2 常规优化

[cite_start]MPI优化与多线程的pthread优化十分相近, 因此只需要将你在pthread里实现的DIF/DIT和CRT合并在多进程里再实现一次即可。 [cite: 23] [cite_start]可以尝试将CRT合并的模数分组计算, 同一进程内使用多个线程进行计算, 最后多个进程再统一合并。 [cite: 23]

[cite_start][Image: 多进程NTT的一种简要实现示意图, 展示了数据通过Scatter分发到不同进程, 计算后再通过Gather合并的过程。] [cite: 23]
