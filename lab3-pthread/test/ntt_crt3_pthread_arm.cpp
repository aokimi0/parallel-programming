#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <numeric> 
#include <algorithm> 
#include <stdexcept>
#include <cmath> 
#include <pthread.h>
#include <thread>

using u64 = uint64_t;

typedef __int128 int128;
typedef long long ll;

const ll M0_CRT = 6597069766657LL;
const ll G0_CRT = 5;
const ll M1_CRT = 2748779069441LL;
const ll G1_CRT = 3;
const ll M2_CRT = 2061584302081LL;
const ll G2_CRT = 7;

const ll CRT_MODS[3] = {M0_CRT, M1_CRT, M2_CRT};
const ll CRT_ROOTS[3] = {G0_CRT, G1_CRT, G2_CRT};

const int MAX_THREADS = 8;
int num_threads = 4;

ll power(ll base, ll exp, ll mod);
ll modInverse(ll n, ll mod);

struct StaticThreadPool {
    pthread_t threads[MAX_THREADS];
    pthread_barrier_t barrier;
    pthread_mutex_t task_mutex;
    pthread_cond_t task_cond;
    
    bool task_ready;
    bool shutdown;
    void (*current_task)(int);
    int active_threads;
    bool initialized;
    
    StaticThreadPool() : task_ready(false), shutdown(false), active_threads(0), initialized(false) {
        pthread_mutex_init(&task_mutex, nullptr);
        pthread_cond_init(&task_cond, nullptr);
    }
    
    ~StaticThreadPool() {
        if (initialized) {
            shutdown = true;
            pthread_cond_broadcast(&task_cond);
            for (int i = 0; i < active_threads; i++) {
                pthread_join(threads[i], nullptr);
            }
            pthread_barrier_destroy(&barrier);
        }
        pthread_mutex_destroy(&task_mutex);
        pthread_cond_destroy(&task_cond);
    }
    
    void init(int n_threads) {
        if (initialized) return;
        
        active_threads = std::min(std::max(n_threads, 1), MAX_THREADS);
        pthread_barrier_init(&barrier, nullptr, active_threads + 1);
        
        for (int i = 0; i < active_threads; i++) {
            pthread_create(&threads[i], nullptr, worker_thread, this);
        }
        initialized = true;
    }
    
    static void* worker_thread(void* arg) {
        StaticThreadPool* pool = (StaticThreadPool*)arg;
        int thread_id = -1;
        
        for (int i = 0; i < pool->active_threads; i++) {
            if (pthread_equal(pthread_self(), pool->threads[i])) {
                thread_id = i;
                break;
            }
        }
        
        while (true) {
            pthread_mutex_lock(&pool->task_mutex);
            while (!pool->task_ready && !pool->shutdown) {
                pthread_cond_wait(&pool->task_cond, &pool->task_mutex);
            }
            
            if (pool->shutdown) {
                pthread_mutex_unlock(&pool->task_mutex);
                break;
            }
            
            void (*task)(int) = pool->current_task;
            pthread_mutex_unlock(&pool->task_mutex);
            
            if (task && thread_id >= 0) {
                task(thread_id);
            }
            
            pthread_barrier_wait(&pool->barrier);
        }
        return nullptr;
    }
    
    void execute_task(void (*task)(int)) {
        if (!initialized) return;
        
        pthread_mutex_lock(&task_mutex);
        current_task = task;
        task_ready = true;
        pthread_cond_broadcast(&task_cond);
        pthread_mutex_unlock(&task_mutex);
        
        pthread_barrier_wait(&barrier);
        
        pthread_mutex_lock(&task_mutex);
        task_ready = false;
        pthread_mutex_unlock(&task_mutex);
    }
};

static StaticThreadPool thread_pool;

struct NTTTaskData {
    std::vector<ll>* a;
    bool invert;
    ll mod;
    ll root;
    int n;
    int len;
};

static NTTTaskData current_ntt_task;

void ntt_worker(int thread_id) {
    std::vector<ll>& a = *(current_ntt_task.a);
    bool invert = current_ntt_task.invert;
    ll mod = current_ntt_task.mod;
    ll root = current_ntt_task.root;
    int n = current_ntt_task.n;
    int len = current_ntt_task.len;
    
    ll wlen = power(root, (mod - 1) / len, mod);
    if (invert)
        wlen = modInverse(wlen, mod);
        
    int num_blocks = n / len;
    int blocks_per_thread = (num_blocks + thread_pool.active_threads - 1) / thread_pool.active_threads;
    int start_block = thread_id * blocks_per_thread;
    int end_block = std::min((thread_id + 1) * blocks_per_thread, num_blocks);
    
    for (int block = start_block; block < end_block; block++) {
        int i = block * len;
        ll w = 1;
        for (int j = 0; j < len / 2; j++) {
            ll u = a[i + j], v = (int128)a[i + j + len / 2] * w % mod;
            a[i + j] = (u + v) % mod;
            a[i + j + len / 2] = (u - v + mod) % mod;
            w = (int128)w * wlen % mod;
        }
    }
}

ll power(ll base, ll exp, ll mod) {
    ll res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (int128)res * base % mod;
        base = (int128)base * base % mod;
        exp /= 2;
    }
    return res;
}

ll modInverse(ll n, ll mod) {
    return power(n, mod - 2, mod);
}

void ntt_pthread(std::vector<ll>& a, bool invert, ll mod, ll root) {
    int n = a.size();
    if (n == 0) return;
    
    thread_pool.init(num_threads);
    
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    
    current_ntt_task.a = &a;
    current_ntt_task.invert = invert;
    current_ntt_task.mod = mod;
    current_ntt_task.root = root;
    current_ntt_task.n = n;
    
    for (int len = 2; len <= n; len <<= 1) {
        current_ntt_task.len = len;
        thread_pool.execute_task(ntt_worker);
    }
    
    if (invert) {
        ll n_inv = modInverse(n, mod);
        for (ll& x : a)
            x = (int128)x * n_inv % mod;
    }
}

ll extended_gcd(ll a, ll b, ll& x, ll& y) {
    if (a == 0) {
        x = 0;
        y = 1;
        return b;
    }
    ll x1, y1;
    ll gcd = extended_gcd(b % a, a, x1, y1);
    x = y1 - (b / a) * x1;
    y = x1;
    return gcd;
}

ll inv_exgcd(ll x, ll m) {
    ll inv_x, y;
    ll g = extended_gcd(x, m, inv_x, y);
    if (g != 1) {
        throw std::runtime_error("Modular inverse does not exist for CRT.");
    }
    return (inv_x % m + m) % m;
}

struct CRTThreadData {
    const std::vector<ll>* poly_a_padded;
    const std::vector<ll>* poly_b_padded;
    std::vector<std::vector<ll>>* ntt_results_mod_crt_M;
    int n_fft;
    int mod_index;
};

void* crt_thread_worker(void* arg) {
    CRTThreadData* data = (CRTThreadData*)arg;
    int k = data->mod_index;
    
    std::vector<ll> temp_a = *(data->poly_a_padded);
    std::vector<ll> temp_b = *(data->poly_b_padded);
    
    for(int i = 0; i < data->n_fft; ++i) {
        temp_a[i] %= CRT_MODS[k];
        if (temp_a[i] < 0) temp_a[i] += CRT_MODS[k];
        temp_b[i] %= CRT_MODS[k];
        if (temp_b[i] < 0) temp_b[i] += CRT_MODS[k];
    }

    ntt_pthread(temp_a, false, CRT_MODS[k], CRT_ROOTS[k]);
    ntt_pthread(temp_b, false, CRT_MODS[k], CRT_ROOTS[k]);

    for (int i = 0; i < data->n_fft; ++i) {
        (*(data->ntt_results_mod_crt_M))[k][i] = (int128)temp_a[i] * temp_b[i] % CRT_MODS[k];
    }
    
    ntt_pthread((*(data->ntt_results_mod_crt_M))[k], true, CRT_MODS[k], CRT_ROOTS[k]);
    
    return nullptr;
}

std::vector<ll> multiply_ntt_crt_for_linear_pthread(const std::vector<ll>& poly_a_orig, const std::vector<ll>& poly_b_orig, int n_orig_coeffs, ll final_p_target) {
    if (n_orig_coeffs == 0) return {};

    int linear_conv_len = 2 * n_orig_coeffs - 1;
    if (n_orig_coeffs == 1) linear_conv_len = 1;
    
    int n_fft = 1;
    while(n_fft < linear_conv_len) n_fft <<= 1;
    if (linear_conv_len == 0) n_fft = 0;

    std::vector<ll> poly_a_padded(n_fft, 0);
    std::vector<ll> poly_b_padded(n_fft, 0);

    for(int i=0; i < n_orig_coeffs; ++i) {
        poly_a_padded[i] = poly_a_orig[i];
        poly_b_padded[i] = poly_b_orig[i];
    }

    std::vector<std::vector<ll>> ntt_results_mod_crt_M(3, std::vector<ll>(n_fft));

    pthread_t threads[3];
    CRTThreadData thread_data[3];
    
    for (int k = 0; k < 3; ++k) {
        thread_data[k].poly_a_padded = &poly_a_padded;
        thread_data[k].poly_b_padded = &poly_b_padded;
        thread_data[k].ntt_results_mod_crt_M = &ntt_results_mod_crt_M;
        thread_data[k].n_fft = n_fft;
        thread_data[k].mod_index = k;
        
        pthread_create(&threads[k], nullptr, crt_thread_worker, &thread_data[k]);
    }
    
    for (int k = 0; k < 3; ++k) {
        pthread_join(threads[k], nullptr);
    }

    std::vector<ll> combined_result_full(n_fft);
    ll m0_inv_m1 = inv_exgcd(CRT_MODS[0], CRT_MODS[1]);
    int128 m0_m1_prod = (int128)CRT_MODS[0] * CRT_MODS[1]; 
    ll m0_m1_mod_m2 = m0_m1_prod % CRT_MODS[2];
    ll m0m1_inv_m2 = inv_exgcd(m0_m1_mod_m2, CRT_MODS[2]);

    for (int i = 0; i < n_fft; ++i) {
        ll c0 = ntt_results_mod_crt_M[0][i];
        ll c1 = ntt_results_mod_crt_M[1][i];
        ll c2 = ntt_results_mod_crt_M[2][i];

        int128 k0_num = (c1 - c0 + CRT_MODS[1]) % CRT_MODS[1];
        int128 k0 = k0_num * m0_inv_m1 % CRT_MODS[1];
        int128 res1 = (c0 + k0 * CRT_MODS[0]);

        int128 c2_minus_res1_mod_m2 = (c2 - (res1 % CRT_MODS[2]) + CRT_MODS[2]) % CRT_MODS[2];
        int128 k1 = c2_minus_res1_mod_m2 * m0m1_inv_m2 % CRT_MODS[2];
        
        int128 current_combined_ans = res1 + k1 * m0_m1_prod;
        combined_result_full[i] = current_combined_ans % final_p_target;
    }
    
    combined_result_full.resize(linear_conv_len > 0 ? linear_conv_len : 0);
    return combined_result_full;
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
    if (!fin.is_open()) { return; }
    fin>>*n>>*p;
    for (int i = 0; i < *n; i++){
        fin>>a[i];
    }
    for (int i = 0; i < *n; i++){   
        fin>>b[i];
    }
    fin.close();
}

void fCheck(u64 *ab, int n_coeffs, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char data_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), data_path);
    data_path[strout.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    if (!fin.is_open()) { std::cout << "Check file not found: " << strout << std::endl; return; }
    int linear_conv_len = 2 * n_coeffs -1;
    if (n_coeffs == 1) linear_conv_len = 1;
    if (n_coeffs == 0) linear_conv_len = 0;

    for (int i = 0; i < linear_conv_len; i++){
        u64 x;
        fin>>x;
        if(fin.fail()) { std::cout << "Failed to read from check file or not enough data." << std::endl; return;}
        if(x != ab[i]){
            std::cout<<"多项式乘法结果错误 at index " << i << " Expected " << x << " Got " << ab[i] <<std::endl;
            fin.close();
            return;
        }
    }
    std::cout<<"多项式乘法结果正确"<<std::endl;
    fin.close();
}

void fWrite(u64 *ab, int n_coeffs, int input_id){
    std::string str1 = "files/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char output_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), output_path);
    output_path[strout.size()] = '\0';
    std::ofstream fout;
    fout.open(output_path, std::ios::out);
    if (!fout.is_open()) { return; }
    int linear_conv_len = 2 * n_coeffs - 1;
    if (n_coeffs == 1) linear_conv_len = 1;
    if (n_coeffs == 0) linear_conv_len = 0;

    for (int i = 0; i < linear_conv_len; i++){
        fout<<ab[i]<<'\n';
    }
    fout.close();
}

void poly_multiply_naive(u64 *a, u64 *b, u64 *ab, int n, u64 p){
    int linear_conv_len = 2 * n - 1;
    if (n == 1) linear_conv_len = 1;
    if (n == 0) {memset(ab, 0, 0); return;}

    memset(ab, 0, sizeof(u64) * linear_conv_len); 

    for(int i = 0; i < n; ++i){
        for(int j = 0; j < n; ++j){
            ab[i+j]=( (u64)a[i] * b[j] % p + ab[i+j]) % p;
        }
    }
}

u64 a[300000], b[300000], ab[600000]; 

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    if (argc > 1) {
        num_threads = std::atoi(argv[1]);
        if (num_threads <= 0) num_threads = 4;
    }
    
    int test_begin = 0;
    int test_end = 4;

    for(int i = test_begin; i <= test_end; ++i){
        long double timing_ans = 0;
        int n_coeffs_from_file; 
        u64 p_target_from_file;
        
        fRead(a, b, &n_coeffs_from_file, &p_target_from_file, i);
        
        int linear_conv_len = (n_coeffs_from_file == 0) ? 0 : (2 * n_coeffs_from_file - 1);
        if (n_coeffs_from_file == 1) linear_conv_len = 1;
        memset(ab, 0, sizeof(u64) * (linear_conv_len > 0 ? linear_conv_len : 1) ); 

        auto Start = std::chrono::high_resolution_clock::now();
        
        if (n_coeffs_from_file > 0) {
            std::vector<ll> poly_a_vec(n_coeffs_from_file);
            std::vector<ll> poly_b_vec(n_coeffs_from_file);
            for(int j=0; j<n_coeffs_from_file; ++j) {
                poly_a_vec[j] = a[j];
                poly_b_vec[j] = b[j];
            }
            
            std::vector<ll> result_vec;
            try {
                result_vec = multiply_ntt_crt_for_linear_pthread(poly_a_vec, poly_b_vec, n_coeffs_from_file, p_target_from_file);
            } catch (const std::exception& e) {
                 std::cerr << "Exception during CRT NTT: " << e.what() << " for test case " << i << std::endl;
                 return 1; 
            }

            for(int j=0; j<result_vec.size() && j < linear_conv_len; ++j) {
                ab[j] = result_vec[j];
            }
        }

        auto End = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli>elapsed = End - Start; 
        timing_ans += elapsed.count();
        
        std::cout << "Test case " << i << ": n_coeffs = " << n_coeffs_from_file << ", p_target = " << p_target_from_file << std::endl;
        fCheck(ab, n_coeffs_from_file, i);
        std::cout<<"average latency: "<<timing_ans<<" (ms) "<<std::endl; 
        fWrite(ab, n_coeffs_from_file, i); 
    }
    return 0;
} 