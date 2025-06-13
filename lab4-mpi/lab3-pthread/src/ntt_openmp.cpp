#include "common_crt_ntt.h"
#include <vector>
#include <omp.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <stdexcept>

// OpenMP优化的NTT/INTT
void ntt_transform_omp(std::vector<long long>& a, bool invert, long long mod, long long primitive_root, int num_threads) {
    int n = a.size();
    if (n == 0) return;
    if (num_threads <= 0) num_threads = 1;
    omp_set_num_threads(num_threads);

    // 位逆序置换 (串行)
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    // 蝶形运算
    for (int len = 2; len <= n; len <<= 1) {
        long long wlen_val = power(primitive_root, (mod - 1) / len, mod);
        if (invert) {
            wlen_val = modInverse(wlen_val, mod);
        }
        // 并行化外层 i 的循环 (每个i代表一个长度为len的独立计算块的开始)
        // 变量 w 需要是每个线程私有的，因为它在内层循环中迭代更新
        // u, v 也是每次迭代的局部变量
        #pragma omp parallel for schedule(static) num_threads(num_threads) // 静态调度，因为每个i的块计算量相同
        for (int i = 0; i < n; i += len) {
            long long w = 1; // 每个线程的 w 从1开始
            for (int j = 0; j < len / 2; j++) {
                long long u = a[i + j];
                long long v = ((unsigned __int128)a[i + j + len / 2] * w) % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = ((unsigned __int128)w * wlen_val) % mod;
            }
        }
    }

    if (invert) {
        long long n_inv = modInverse(n, mod);
        // 这个循环也可以并行化
        #pragma omp parallel for num_threads(num_threads)
        for (int i = 0; i < n; ++i) {
            a[i] = ((unsigned __int128)a[i] * n_inv) % mod;
        }
    }
}

// 使用OpenMP优化的NTT进行多项式乘法
std::vector<long long> multiply_ntt_omp(
    std::vector<long long> poly1, 
    std::vector<long long> poly2, 
    long long mod, 
    long long primitive_root, 
    int num_threads) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    int n = 1;
    while (n < n1 + n2 -1) n <<= 1;
    if (n == 0 && (n1 > 0 || n2 > 0) ) n = 1;
    if (n1 == 0 || n2 == 0) return {};

    poly1.resize(n);
    poly2.resize(n);

    ntt_transform_omp(poly1, false, mod, primitive_root, num_threads);
    ntt_transform_omp(poly2, false, mod, primitive_root, num_threads);

    std::vector<long long> result(n);
    #pragma omp parallel for num_threads(num_threads) // 控制这里的线程数
    for (int i = 0; i < n; i++) {
        result[i] = ((unsigned __int128)poly1[i] * poly2[i]) % mod;
    }

    ntt_transform_omp(result, true, mod, primitive_root, num_threads);

    int actual_size = 0;
    if (n1 > 0 && n2 > 0) {
        actual_size = n1 + n2 - 1;
    }
    if (actual_size > 0) {
        result.resize(actual_size);
    } else {
        result.clear();
    }
    return result;
}

// 辅助函数，用于打印结果
void print_polynomial_output_for_omp_main(const std::vector<long long>& p) {
    for (size_t i = 0; i < p.size(); ++i) {
        std::cout << p[i] << (i == p.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int num_threads = 1;
    if (argc > 1) {
        try {
            num_threads = std::stoi(argv[1]);
            if (num_threads <= 0) {
                num_threads = 1;
                // std::cerr << "警告: 线程数必须为正。使用默认值 1。\n"; // Suppress warnings
            }
        } catch (const std::exception& e) {
            // std::cerr << "警告: 无法解析线程数参数 '" << argv[1] << "'。使用默认值 1. 错误: " << e.what() << "\n";
            num_threads = 1;
        }
    }

    int n_coeffs; 
    long long mod_val;
    
    if (!(std::cin >> n_coeffs >> mod_val)) {
        std::cerr << "Error: Failed to read N_COEFFS and MOD from stdin. (openmp)" << std::endl;
        return 1;
    }

    if (n_coeffs <= 0) {
        std::cerr << "Error: N_COEFFS must be positive. Got " << n_coeffs << " (openmp)" << std::endl;
        return 1;
    }
    
    std::vector<long long> p1(n_coeffs); 
    std::vector<long long> p2(n_coeffs); 

    for (int i = 0; i < n_coeffs; ++i) { 
        if (!(std::cin >> p1[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial A. (openmp)" << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < n_coeffs; ++i) { 
        if (!(std::cin >> p2[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial B. (openmp)" << std::endl;
            return 1;
        }
    }

    long long primitive_root = 3; 
    try {
        std::vector<long long> result = multiply_ntt_omp(p1, p2, mod_val, primitive_root, num_threads);
        print_polynomial_output_for_omp_main(result);
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error in ntt_openmp: " << e.what() << std::endl;
        return 1; // Exit with error code
    }

    return 0;
} 