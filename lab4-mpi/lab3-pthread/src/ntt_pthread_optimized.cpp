#include "common_crt_ntt.h"
#include <vector>
#include <pthread.h>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>

struct ThreadPoolTask {
    std::vector<long long>* vec_a;
    int n;
    int len;
    long long wlen;
    long long mod;
    int start_block_idx;
    int end_block_idx;
    bool task_ready;
    bool should_exit;
    
    ThreadPoolTask() : task_ready(false), should_exit(false) {}
};

struct ThreadPool {
    pthread_t* threads;
    ThreadPoolTask* tasks;
    pthread_mutex_t* task_mutexes;
    pthread_cond_t* task_conditions;
    pthread_mutex_t sync_mutex;
    pthread_cond_t sync_condition;
    int num_threads;
    int active_threads;
    bool initialized;
    
    ThreadPool() : threads(nullptr), tasks(nullptr), task_mutexes(nullptr), 
                   task_conditions(nullptr), num_threads(0), active_threads(0), initialized(false) {}
};

static ThreadPool global_thread_pool;

void cleanup_thread_pool();

void* thread_pool_worker(void* arg) {
    int thread_id = *static_cast<int*>(arg);
    ThreadPoolTask* task = &global_thread_pool.tasks[thread_id];
    pthread_mutex_t* task_mutex = &global_thread_pool.task_mutexes[thread_id];
    pthread_cond_t* task_cond = &global_thread_pool.task_conditions[thread_id];
    
    while (true) {
        pthread_mutex_lock(task_mutex);
        while (!task->task_ready && !task->should_exit) {
            pthread_cond_wait(task_cond, task_mutex);
        }
        
        if (task->should_exit) {
            pthread_mutex_unlock(task_mutex);
            break;
        }
        
        std::vector<long long>& a = *(task->vec_a);
        long long mod = task->mod;
        int current_len = task->len;
        long long w_len_val = task->wlen;
        
        for (int block_idx = task->start_block_idx; block_idx < task->end_block_idx; ++block_idx) {
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
        
        task->task_ready = false;
        pthread_mutex_unlock(task_mutex);
        
        pthread_mutex_lock(&global_thread_pool.sync_mutex);
        global_thread_pool.active_threads--;
        if (global_thread_pool.active_threads == 0) {
            pthread_cond_signal(&global_thread_pool.sync_condition);
        }
        pthread_mutex_unlock(&global_thread_pool.sync_mutex);
    }
    
    pthread_exit(NULL);
}

void init_thread_pool(int num_threads) {
    if (global_thread_pool.initialized && global_thread_pool.num_threads == num_threads) {
        return; 
    }
    
    if (global_thread_pool.initialized) {
        cleanup_thread_pool();
    }
    
    global_thread_pool.num_threads = num_threads;
    global_thread_pool.threads = new pthread_t[num_threads];
    global_thread_pool.tasks = new ThreadPoolTask[num_threads];
    global_thread_pool.task_mutexes = new pthread_mutex_t[num_threads];
    global_thread_pool.task_conditions = new pthread_cond_t[num_threads];
    
    pthread_mutex_init(&global_thread_pool.sync_mutex, NULL);
    pthread_cond_init(&global_thread_pool.sync_condition, NULL);
    
    for (int i = 0; i < num_threads; i++) {
        pthread_mutex_init(&global_thread_pool.task_mutexes[i], NULL);
        pthread_cond_init(&global_thread_pool.task_conditions[i], NULL);
    }
    
    static int* thread_ids = new int[num_threads]; 
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&global_thread_pool.threads[i], NULL, thread_pool_worker, &thread_ids[i]);
    }
    
    global_thread_pool.initialized = true;
}

void cleanup_thread_pool() {
    if (!global_thread_pool.initialized) return;
    
    for (int i = 0; i < global_thread_pool.num_threads; i++) {
        pthread_mutex_lock(&global_thread_pool.task_mutexes[i]);
        global_thread_pool.tasks[i].should_exit = true;
        pthread_cond_signal(&global_thread_pool.task_conditions[i]);
        pthread_mutex_unlock(&global_thread_pool.task_mutexes[i]);
    }
    
    for (int i = 0; i < global_thread_pool.num_threads; i++) {
        pthread_join(global_thread_pool.threads[i], NULL);
    }
    
    for (int i = 0; i < global_thread_pool.num_threads; i++) {
        pthread_mutex_destroy(&global_thread_pool.task_mutexes[i]);
        pthread_cond_destroy(&global_thread_pool.task_conditions[i]);
    }
    
    pthread_mutex_destroy(&global_thread_pool.sync_mutex);
    pthread_cond_destroy(&global_thread_pool.sync_condition);
    
    delete[] global_thread_pool.threads;
    delete[] global_thread_pool.tasks;
    delete[] global_thread_pool.task_mutexes;
    delete[] global_thread_pool.task_conditions;
    
    global_thread_pool.initialized = false;
}

void ntt_transform_pthread_optimized(std::vector<long long>& a, bool invert, long long mod, long long primitive_root, int num_threads) {
    int n = a.size();
    if (num_threads <= 0) num_threads = 1;
    if (n == 0) return;
    
    init_thread_pool(num_threads);
    
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    
    for (int len_iter = 2; len_iter <= n; len_iter <<= 1) {
        long long wlen_val = power(primitive_root, (mod - 1) / len_iter, mod);
        if (invert) {
            wlen_val = modInverse(wlen_val, mod);
        }
        
        int num_blocks_total = n / len_iter;
        if (num_blocks_total == 0) continue;
        
        int actual_threads_to_use = std::min(num_threads, num_blocks_total);
        if (actual_threads_to_use <= 0) actual_threads_to_use = 1;
        
        global_thread_pool.active_threads = actual_threads_to_use;
        
        for (int t = 0; t < actual_threads_to_use; ++t) {
            ThreadPoolTask* task = &global_thread_pool.tasks[t];
            
            task->vec_a = &a;
            task->n = n;
            task->len = len_iter;
            task->wlen = wlen_val;
            task->mod = mod;
            
            int blocks_per_thread = (num_blocks_total + actual_threads_to_use - 1) / actual_threads_to_use;
            task->start_block_idx = t * blocks_per_thread;
            task->end_block_idx = std::min(num_blocks_total, (t + 1) * blocks_per_thread);
            
            pthread_mutex_lock(&global_thread_pool.task_mutexes[t]);
            task->task_ready = true;
            pthread_cond_signal(&global_thread_pool.task_conditions[t]);
            pthread_mutex_unlock(&global_thread_pool.task_mutexes[t]);
        }
        
        pthread_mutex_lock(&global_thread_pool.sync_mutex);
        while (global_thread_pool.active_threads > 0) {
            pthread_cond_wait(&global_thread_pool.sync_condition, &global_thread_pool.sync_mutex);
        }
        pthread_mutex_unlock(&global_thread_pool.sync_mutex);
    }
    
    if (invert) {
        long long n_inv = modInverse(n, mod);
        for (long long& x : a) {
            x = ((__int128)x * n_inv) % mod;
        }
    }
}

std::vector<long long> multiply_ntt_pthread_optimized(
    std::vector<long long> poly1, 
    std::vector<long long> poly2, 
    long long mod, 
    long long primitive_root, 
    int num_threads) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    int n = 1;
    while (n < n1 + n2 - 1) n <<= 1;
    if (n == 0 && (n1 > 0 || n2 > 0)) n = 1;
    if (n1 == 0 || n2 == 0) return {};
    
    poly1.resize(n);
    poly2.resize(n);
    
    ntt_transform_pthread_optimized(poly1, false, mod, primitive_root, num_threads);
    ntt_transform_pthread_optimized(poly2, false, mod, primitive_root, num_threads);
    
    std::vector<long long> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = ((__int128)poly1[i] * poly2[i]) % mod;
    }
    
    ntt_transform_pthread_optimized(result, true, mod, primitive_root, num_threads);
    
    if (n1 == 0 || n2 == 0) {
        result.clear();
    } else {
        int final_size = n1 + n2 - 1;
        if (final_size <= 0 && (n1 > 0 || n2 > 0)) final_size = std::max(n1, n2);
        if (final_size > 0) result.resize(final_size);
        else result.clear();
    }
    return result;
}

void cleanup_at_exit() {
    cleanup_thread_pool();
}

void print_polynomial_output_for_pthread_optimized_main(const std::vector<long long>& p) {
    for (size_t i = 0; i < p.size(); ++i) {
        std::cout << p[i] << (i == p.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    atexit(cleanup_at_exit); 
    
    int num_threads = 1;
    if (argc > 1) {
        try {
            num_threads = std::stoi(argv[1]);
            if (num_threads <= 0) {
                num_threads = 1;
            }
        } catch (const std::exception& e) {
            num_threads = 1;
        }
    }
    
    int n_coeffs;
    long long mod_val;
    
    if (!(std::cin >> n_coeffs >> mod_val)) {
        std::cerr << "Error: Failed to read N_COEFFS and MOD from stdin. (pthread_optimized)" << std::endl;
        return 1;
    }

    if (n_coeffs <= 0) {
        std::cerr << "Error: N_COEFFS must be positive. Got " << n_coeffs << " (pthread_optimized)" << std::endl;
        return 1;
    }
    
    std::vector<long long> p1(n_coeffs);
    std::vector<long long> p2(n_coeffs);
    
    for (int i = 0; i < n_coeffs; ++i) { 
        if (!(std::cin >> p1[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial A. (pthread_optimized)" << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < n_coeffs; ++i) { 
        if (!(std::cin >> p2[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial B. (pthread_optimized)" << std::endl;
            return 1;
        }
    }
    
    long long primitive_root = 3;
    std::atexit(cleanup_at_exit); // 注册退出处理函数
    try {
        std::vector<long long> result = multiply_ntt_pthread_optimized(p1, p2, mod_val, primitive_root, num_threads);
        print_polynomial_output_for_pthread_optimized_main(result);
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error in ntt_pthread_optimized: " << e.what() << std::endl;
        return 1; // Exit with error code
    }
    
    return 0;
} 