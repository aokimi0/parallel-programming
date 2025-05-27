#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sys/time.h>
#include <cstdint>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <pthread.h>

using u64 = uint64_t;


long long power(long long base, long long exp, long long mod) {
    long long res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = ((__int128)res * base) % mod;
        base = ((__int128)base * base) % mod;
        exp /= 2;
    }
    return res;
}

long long inverse(long long n, long long mod) {
    return power(n, mod - 2, mod);
}

struct NttThreadData {
    std::vector<long long>* vec_a;
    int n;
    int len;
    long long wlen;
    long long mod;
    bool invert;
    int thread_id;
    int num_threads;
    int start_block_idx;
    int end_block_idx;
};

void* ntt_butterfly_thread_func(void* arg) {
    NttThreadData* data = static_cast<NttThreadData*>(arg);
    std::vector<long long>& a = *(data->vec_a);
    long long mod = data->mod;
    int n = data->n;
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

    pthread_t* threads = new pthread_t[num_threads];
    NttThreadData* thread_data_array = new NttThreadData[num_threads];

    for (int len_iter = 2; len_iter <= n; len_iter <<= 1) {
        long long wlen_val = power(primitive_root, (mod - 1) / len_iter, mod);
        if (invert) {
            wlen_val = inverse(wlen_val, mod);
        }

        int num_blocks_total = n / len_iter;
        int actual_threads_to_use = std::min(num_threads, num_blocks_total);
        if (actual_threads_to_use <= 0 && num_blocks_total > 0) actual_threads_to_use = 1;

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
    }

    delete[] threads;
    delete[] thread_data_array;

    if (invert) {
        long long n_inv = inverse(n, mod);
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
    int num_threads) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    if (n1 == 0 || n2 == 0) return {};
    
    int n = 1;
    while (n < n1 + n2 - 1) n <<= 1;

    poly1.resize(n);
    poly2.resize(n);

    ntt_transform_pthread(poly1, false, mod, primitive_root, num_threads);
    ntt_transform_pthread(poly2, false, mod, primitive_root, num_threads);

    std::vector<long long> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = ((__int128)poly1[i] * poly2[i]) % mod;
    }

    ntt_transform_pthread(result, true, mod, primitive_root, num_threads);
    result.resize(n1 + n2 - 1);
    
    return result;
}

void fRead(u64 *a, u64 *b, int *n, u64 *p, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    char data_path[strin.size() + 1];
    std::copy(strin.begin(), strin.end(), data_path);
    data_path[strin.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    fin>>*n>>*p;
    for (int i = 0; i < *n; i++){
        fin>>a[i];
    }
    for (int i = 0; i < *n; i++){   
        fin>>b[i];
    }
}

void fCheck(u64 *ab, int n, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char data_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), data_path);
    data_path[strout.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    for (int i = 0; i < n * 2 - 1; i++){
        u64 x;
        fin>>x;
        if(x != ab[i]){
            std::cout<<"多项式乘法结果错误"<<std::endl;
            return;
        }
    }
    std::cout<<"多项式乘法结果正确"<<std::endl;
    return;
}

void fWrite(u64 *ab, int n, int input_id){
    std::string str1 = "files/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + "_pthread_arm.out";
    char output_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), output_path);
    output_path[strout.size()] = '\0';
    std::ofstream fout;
    fout.open(output_path, std::ios::out);
    for (int i = 0; i < n * 2 - 1; i++){
        fout<<ab[i]<<'\n';
    }
}

void ntt_multiply_wrapper(u64 *a, u64 *b, u64 *ab, int n, u64 p, int num_threads = 4){
    std::vector<long long> poly1(n), poly2(n);
    for(int i = 0; i < n; i++){
        poly1[i] = a[i];
        poly2[i] = b[i];
    }
    
    long long primitive_root = 3; 
    std::vector<long long> result = multiply_ntt_pthread(poly1, poly2, p, primitive_root, num_threads);
    
    for(int i = 0; i < n * 2 - 1; i++){
        ab[i] = result[i];
    }
}

u64 a[300000], b[300000], ab[300000];
int main(int argc, char *argv[])
{
    std::cout << "=== NTT Pthread ARM 版本测试 ===" << std::endl;
    
    int num_threads = 4; // 默认4线程
    if(argc > 1){
        num_threads = std::atoi(argv[1]);
        if(num_threads <= 0) num_threads = 4;
    }
    std::cout << "使用线程数: " << num_threads << std::endl;
    
    int test_begin = 0;
    int test_end = 4;
    for(int i = test_begin; i <= test_end; ++i){
        long double ans = 0;
        int n_;
        u64 p_;
        fRead(a, b, &n_, &p_, i);
        memset(ab, 0, sizeof(ab));
        auto Start = std::chrono::high_resolution_clock::now();
        ntt_multiply_wrapper(a, b, ab, n_, p_, num_threads);
        auto End = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::ratio<1,1000>>elapsed = End - Start;
        ans += elapsed.count();
        fCheck(ab, n_, i);
        std::cout<<"average latency for n = "<<n_<<" p = "<<p_<<" : "<<ans<<" (ms) "<<std::endl;
        fWrite(ab, n_, i);
    }
    return 0;
} 