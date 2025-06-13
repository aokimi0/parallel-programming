#include "common_crt_ntt.h"
#include <vector>
#include <pthread.h>
#include <numeric>
#include <algorithm>

struct NttThreadData {
    std::vector<long long>* vec_a; // 指向多项式系数向量的指针
    int n;                        // 多项式长度 (2的幂)
    int len;                      // 当前蝶形运算的长度
    long long wlen;               // 当前轮次的单位根 w_len
    long long mod;                // 模数
    bool invert;                  // 是否是 INTT (虽然wlen已经处理了invert, 但某些逻辑可能需要)
    int thread_id;
    int num_threads;
    int start_block_idx;          // 该线程负责的起始 block 索引
    int end_block_idx;            // 该线程负责的结束 block 索引 (不包含)
};

// 线程执行的蝶形运算函数 (针对特定 len)
void* ntt_butterfly_thread_func(void* arg) {
    NttThreadData* data = static_cast<NttThreadData*>(arg);
    std::vector<long long>& a = *(data->vec_a);
    long long mod = data->mod;
    // int n = data->n; // Unused variable
    int current_len = data->len;
    long long w_len_val = data->wlen;

    for (int block_idx = data->start_block_idx; block_idx < data->end_block_idx; ++block_idx) {
        int i = block_idx * current_len;
        long long w = 1;
        for (int j = 0; j < current_len / 2; j++) {
            long long u = a[i + j];
            long long v = ((__int128)a[i + j + current_len / 2] * w) % mod;
            a[i + j] = (u + v) % mod;
            a[i + j + current_len / 2] = (u - v + mod) % mod;
            w = ((__int128)w * w_len_val) % mod;
        }
    }
    pthread_exit(NULL);
}

void ntt_transform_pthread(std::vector<long long>& a, bool invert, long long mod, long long primitive_root, int num_threads) {
    int n = a.size();
    if (num_threads <= 0) num_threads = 1;
    if (n == 0) return;

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    pthread_t* threads = nullptr;
    NttThreadData* thread_data_array = nullptr;

    for (int len_iter = 2; len_iter <= n; len_iter <<= 1) {
        long long wlen_val = power(primitive_root, (mod - 1) / len_iter, mod);
        if (invert) {
            wlen_val = modInverse(wlen_val, mod);
        }

        int num_blocks_total = n / len_iter;
        if (num_blocks_total == 0) {
            continue;
        }

        int actual_threads_to_use = std::min(num_threads, num_blocks_total);
        if (actual_threads_to_use <= 0) actual_threads_to_use = 1;
        
        try {
            threads = new pthread_t[actual_threads_to_use];
            thread_data_array = new NttThreadData[actual_threads_to_use];
        } catch (const std::bad_alloc& e) {
            std::cerr << "Failed to allocate memory for threads or thread_data: " << e.what() << std::endl;
            if (threads) delete[] threads;
            if (thread_data_array) delete[] thread_data_array;
            throw;
        }

        for (int t = 0; t < actual_threads_to_use; ++t) {
            thread_data_array[t].vec_a = &a;
            thread_data_array[t].n = n;
            thread_data_array[t].len = len_iter;
            thread_data_array[t].wlen = wlen_val;
            thread_data_array[t].mod = mod;
            thread_data_array[t].invert = invert;
            thread_data_array[t].thread_id = t;
            thread_data_array[t].num_threads = actual_threads_to_use;

            int blocks_per_thread_ideal = (num_blocks_total + actual_threads_to_use - 1) / actual_threads_to_use;
            thread_data_array[t].start_block_idx = t * blocks_per_thread_ideal;
            thread_data_array[t].end_block_idx = std::min(num_blocks_total, (t + 1) * blocks_per_thread_ideal);

            if (thread_data_array[t].start_block_idx < thread_data_array[t].end_block_idx) {
                 pthread_create(&threads[t], NULL, ntt_butterfly_thread_func, &thread_data_array[t]);
            }
        }

        for (int t = 0; t < actual_threads_to_use; ++t) {
            if (thread_data_array[t].start_block_idx < thread_data_array[t].end_block_idx) {
                pthread_join(threads[t], NULL);
            }
        }
        
        delete[] threads;
        threads = nullptr;
        delete[] thread_data_array;
        thread_data_array = nullptr;
    }

    if (invert) {
        long long n_inv = modInverse(n, mod);
        for (long long& x : a) {
            x = ((__int128)x * n_inv) % mod;
        }
    }
}

std::vector<long long> multiply_ntt_pthread(
    std::vector<long long> poly1, 
    std::vector<long long> poly2, 
    long long mod, 
    long long primitive_root, 
    int num_threads_param) {
    
    if (num_threads_param <= 0) num_threads_param = 1;

    int n1 = poly1.size();
    int n2 = poly2.size();
    int n = 1;
    while (n < n1 + n2 -1) n <<= 1;
    if (n == 0 && (n1 > 0 || n2 > 0) ) n = 1;
    if (n1 == 0 || n2 == 0) return {};

    poly1.resize(n);
    poly2.resize(n);

    ntt_transform_pthread(poly1, false, mod, primitive_root, num_threads_param);
    ntt_transform_pthread(poly2, false, mod, primitive_root, num_threads_param);

    std::vector<long long> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = ((__int128)poly1[i] * poly2[i]) % mod;
    }

    ntt_transform_pthread(result, true, mod, primitive_root, num_threads_param);

    if (n1 == 0 || n2 == 0) {
        result.clear();
    } else {
        int final_size = n1 + n2 - 1;
        if (final_size <= 0 && (n1>0 || n2>0)) final_size = std::max(n1,n2);
        if (final_size > 0) result.resize(final_size);
        else result.clear();
    }
    return result;
}


#include <iostream> 
#include <string>    
#include <stdexcept> 

void print_polynomial_output_for_pthread_main(const std::vector<long long>& p) {
    for (size_t i = 0; i < p.size(); ++i) {
        std::cout << p[i] << (i == p.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int num_threads_arg = 1;
    if (argc > 1) {
        try {
            num_threads_arg = std::stoi(argv[1]);
            if (num_threads_arg <= 0) {
                // std::cerr << "Warning: Number of threads must be positive. Using 1 thread.\n";
                num_threads_arg = 1;
            }
        } catch (const std::exception& e) {
            // std::cerr << "Warning: Unable to parse thread count argument '" << argv[1] << "'. Using default value 1. Error: " << e.what() << "\n";
            num_threads_arg = 1;
        }
    }

    int n_coeffs; // Changed from n_degree
    long long mod_val;
    
    if (!(std::cin >> n_coeffs >> mod_val)) {
        std::cerr << "Error: Failed to read N_COEFFS and MOD from stdin. (pthread)" << std::endl;
        return 1;
    }
    
    if (n_coeffs <= 0) {
        std::cerr << "Error: N_COEFFS must be positive. Got " << n_coeffs << " (pthread)" << std::endl;
        return 1;
    }

    std::vector<long long> p1(n_coeffs); // Size is n_coeffs
    std::vector<long long> p2(n_coeffs); // Size is n_coeffs

    for (int i = 0; i < n_coeffs; ++i) { // Loop up to n_coeffs-1
        if (!(std::cin >> p1[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial A. (pthread)" << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < n_coeffs; ++i) { // Loop up to n_coeffs-1
        if (!(std::cin >> p2[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial B. (pthread)" << std::endl;
            return 1;
        }
    }

    long long primitive_root = 3; 
    try {
        std::vector<long long> result = multiply_ntt_pthread(p1, p2, mod_val, primitive_root, num_threads_arg);
        print_polynomial_output_for_pthread_main(result);
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error in ntt_pthread: " << e.what() << std::endl;
        return 1; // Exit with error code
    }

    return 0;
} 