# pthread/openmp 多线程优化 NTT

本次多线程实验需要实现朴素算法和 CRT 算法。
NTT 选题要求在本次实验不允许使用 SIMD 优化, 但你可以把多线程
和向量化的结合放在期末报告中。
对于基础版的 NTT 多线程优化, 实现方法较为简单, 由于第二三层循
环相当于遍历了一遍多项式数组, 显然可以对第三层循环进行多线程优化,
类似高斯消元, 因此代码实现较为简单, 只需要注意线程同步正确。

提供一个全新的多线程优化思路, 在 SIMD 的实验指导书中提到了大模
数的 NTT 方法, 其本质即任意模数 NTT, 如果你是在洛谷或 oiwiki 中学习
的 NTT 基础知识, 也能看到任意模数 NTT 的原理, 即使用中国剩余定理 (
CRT ) 合并大模数。
对于任意模数 NTT 的模板, 通常采用三模数 NTT 合并, CRT 多线程
优化的思想类似任意模数 NTT, 如果让每一个线程都使用不同的小模数, 最
终使用 CRT 将结果合并。
如果要实现此多线程优化, 需要在实现多模数 NTT 合并后才能实现
pthread 优化, 当模数越大时, 多线程的优化效果越明显。
你可以仿照任意模数 NTT(三模数合并) 的模板仿推出任意模数 NTT(多
模数合并) 的公式, 也可以自己上网寻找相关博客, 另外以下给出了两篇应
用了 CRT 合并 NTT 以加速大模数的同态加密论文, 有更详细的原理和优
化的证明, 当然 CRT 合并 NTT 的重要性在超大模数 (往往远大于 64 bit)
时才明显, 对于同态加密库 (CKKS 等), 会经常使用超大模数进行加密, 因
此如果实现了本优化, 要求最终需要测试一个大于 32 bit 的模数。
