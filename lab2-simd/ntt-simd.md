# SIMD编程实验

## 实验报告
撰写符合科技论文写作规范的研究报告，内容涵盖问题描述（针对自主选题，先简要描述期末研究报告的大问题，再具体描述本次SIMD编程实验涉及的子问题）、SIMD算法设计（最好包含复杂性分析）与实现（伪代码）、实验及结果分析，并附上Git项目链接。报告不超过15页，重点在于算法设计以及实验结果和分析。

## NEON的C/C++编程
使用NEON Intrinsics进行编程，需包含头文件#include <arm_neon.h> ，编译选项为 -march=native或 -march=armv8-a。介绍了一些常用的NEON指令，包括数据类型、load、store、move、arithmetic等指令，并给出了一个使用NEON Intrinsics计算数组元素和的简单示例。详细完整的NEON Intrinsic函数说明可查询ARM官网文档。
## SSE/AVX的C/C++编程
SSE指令对应C/C++的intrinsics ，使用SSE intrinsics所需头文件较多，如#include <xmmintrin.h>（SSE）、#include <emmintrin.h>（SSE2）等。编译选项有 -march=corei7、 -march=corei7-avx、 -march=native等。介绍了常用的SSE指令，包括数据类型、load、set、store、数据计算等指令，AVX指令与SSE类似。给出了一个SSE优化数组加法的简单示例，更多SSE/AVX指令及AVX编程可参考课程讲义，所有SSE/AVX指令细节可查询官网文档。
## 程序编译及运行
可参考各选题指导书进行程序编译及运行相关操作。
## 使用VTune等工具剖析程序性能
类似之前的实验，使用perf、VTune等profiling工具分析对比NEON、SSE/AVX优化与一般串行、对齐与不对齐等策略下程序执行的指令数、周期数、CPI等性能指标，具体使用方法参考体系结构调研相关及性能测试实验指导书。
## 分析汇编代码
通过研究汇编代码，能更深入理解程序性能产生的原因及优化方向。可使用godbolt分析程序的汇编代码，具体使用方法参考体系结构调研相关实验指导书。

## NTT选题：SIMD实验
### 向量化取模
NTT的SIMD优化中向量化取模有一定难度，neon的向量化操作不支持取模，需要自行实现。若无法完成向量化取模甚至多项式乘法，可采用以下解决方案：涉及取模时单独处理向量里的每一个元素；使用浮点数除法近似取模操作；使用Montgomery规约将模乘转化为支持向量化的操作。介绍了浮点数近似取模和Montgomery规约的原理，Montgomery规约实现难度相对较大，若参考已有代码或仓库需在报告中指出。
### 向量化蝴蝶变换
给出了蝴蝶变换的主体代码，其第三层循环可进行SIMD优化，当步长小于向量长度时用朴素方法计算，步长大于等于向量长度时使用向量优化。还介绍了较难的算法DIT/DIF，DIT按时间抽取，常见实现为Cooley - Tukey算法；DIF按频率抽取，使用DIT和DIF可减少位翻转操作，但需注意向量内部的变换。 