#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <string>

#include <cuda_runtime.h>
#include "common.h"

std::vector<ll> multiply_ntt_serial(std::vector<ll> p1, std::vector<ll> p2, ll mod, ll primitive_root);

#define CUDA_CHECK(err) { \
    cudaError_t err_ = (err); \
    if (err_ != cudaSuccess) { \
        std::cerr << "CUDA error in " << __FILE__ << " at line " << __LINE__ \
                  << ": " << cudaGetErrorString(err_) << std::endl; \
        exit(EXIT_FAILURE); \
    } \
}

__global__ void pointwise_mult_kernel(ll* out, const ll* in1, const ll* in2, int n, ll mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = ((int128)in1[idx] * in2[idx]) % mod;
    }
}

__global__ void final_scaling_kernel(ll* a, int n, ll n_inv, ll mod) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        a[idx] = ((int128)a[idx] * n_inv) % mod;
    }
}

__global__ void ntt_kernel_naive(ll* a, const ll* twiddles, int len, int n, ll mod) {
    int tidx = blockIdx.x * blockDim.x + threadIdx.x;
    int butterfly_grp_idx = tidx / (len / 2);
    int butterfly_idx_in_grp = tidx % (len / 2);
    int i = butterfly_grp_idx * len + butterfly_idx_in_grp;

    if (i < n) {
        ll w = twiddles[butterfly_idx_in_grp];
        ll u = a[i];
        ll v = ((int128)a[i + len / 2] * w) % mod;
        a[i] = (u + v) % mod;
        a[i + len / 2] = (u - v + mod) % mod;
    }
}

template <typename Reducer>
__global__ void pointwise_mult_kernel_optimized(ll* out, const ll* in1, const ll* in2, int n, const Reducer reducer) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = reducer.multiply(in1[idx], in2[idx]);
    }
}

template <typename Reducer>
__global__ void final_scaling_kernel_optimized(ll* a, int n, ll n_inv, const Reducer reducer) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        a[idx] = reducer.multiply(a[idx], n_inv);
    }
}

template<typename Reducer>
__global__ void ntt_kernel_optimized(ll* a, const ll* twiddles, int len, int n, const Reducer reducer) {
    int tidx = blockIdx.x * blockDim.x + threadIdx.x;
    int butterfly_grp_idx = tidx / (len / 2);
    int butterfly_idx_in_grp = tidx % (len / 2);
    int i = butterfly_grp_idx * len + butterfly_idx_in_grp;
    
    if (i < n) {
        ll w = twiddles[butterfly_idx_in_grp];
        ll u = a[i];
        ll v = reducer.multiply(a[i + len / 2], w);
        
        ll sum = u + v;
        a[i] = (sum >= reducer.mod) ? (sum - reducer.mod) : sum;
        
        ll diff = u - v;
        a[i + len / 2] = (diff < 0) ? (diff + reducer.mod) : diff;
    }
}

template<typename Reducer>
__global__ void from_mont_kernel(ll* a, int n, const Reducer reducer) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        a[idx] = reducer.from_mont(a[idx]);
    }
}

template<typename Reducer>
__global__ void to_mont_kernel(ll* a, int n, const Reducer reducer) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        a[idx] = reducer.to_mont(a[idx]);
    }
}

void bit_reverse_copy(std::vector<ll>& a) {
    int n = a.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }
}

void ntt_gpu_naive(ll* d_a, int n, bool invert, ll mod, ll root) {
    int threads_per_block = 256;
    for (int len = 2; len <= n; len <<= 1) {
        std::vector<ll> h_twiddles(len / 2);
        ll wlen = power(root, (mod - 1) / len, mod);
        if (invert) wlen = modInverse(wlen, mod);
        
        h_twiddles[0] = 1;
        for (int j = 1; j < len / 2; j++) {
            h_twiddles[j] = ((int128)h_twiddles[j - 1] * wlen) % mod;
        }
        
        ll* d_twiddles;
        CUDA_CHECK(cudaMalloc((void**)&d_twiddles, (len / 2) * sizeof(ll)));
        CUDA_CHECK(cudaMemcpy(d_twiddles, h_twiddles.data(), (len / 2) * sizeof(ll), cudaMemcpyHostToDevice));

        int num_threads = n / 2;
        int num_blocks = (num_threads + threads_per_block - 1) / threads_per_block;
        ntt_kernel_naive<<<num_blocks, threads_per_block>>>(d_a, d_twiddles, len, n, mod);
        CUDA_CHECK(cudaGetLastError());
        
        CUDA_CHECK(cudaFree(d_twiddles));
    }
}

template<typename Reducer>
void ntt_gpu_optimized(ll* d_a, int n, bool invert, const Reducer& reducer) {
    int threads_per_block = 256;
    
    std::vector<ll> h_all_twiddles;
    size_t total_twiddles = 0;
    for (int len = 2; len <= n; len <<= 1) {
        total_twiddles += len / 2;
    }
    h_all_twiddles.reserve(total_twiddles);

    ll root = power(3, (reducer.mod - 1) / n, reducer.mod);
    if (invert) {
        root = modInverse(root, reducer.mod);
    }
    
    for (int len = 2; len <= n; len <<= 1) {
        ll wlen_base = power(root, n / len, reducer.mod);
        ll w = 1;
        for (int j = 0; j < len / 2; j++) {
            h_all_twiddles.push_back(w);
            w = ((int128)w * wlen_base) % reducer.mod;
        }
    }
    
    ll* d_all_twiddles;
    CUDA_CHECK(cudaMalloc((void**)&d_all_twiddles, total_twiddles * sizeof(ll)));
    CUDA_CHECK(cudaMemcpy(d_all_twiddles, h_all_twiddles.data(), total_twiddles * sizeof(ll), cudaMemcpyHostToDevice));

    size_t twiddle_offset = 0;
    for (int len = 2; len <= n; len <<= 1) {
        ll* d_twiddles_stage = d_all_twiddles + twiddle_offset;
        
        int num_threads = n / 2;
        int num_blocks = (num_threads + threads_per_block - 1) / threads_per_block;
        
        ntt_kernel_optimized<Reducer><<<num_blocks, threads_per_block>>>(d_a, d_twiddles_stage, len, n, reducer);
        CUDA_CHECK(cudaGetLastError());
        
        twiddle_offset += len / 2;
    }

    CUDA_CHECK(cudaFree(d_all_twiddles));
}

template<typename Reducer>
void ntt_gpu_montgomery(ll* d_a, int n, bool invert, const Reducer& reducer) {
    int threads_per_block = 256;
    
    std::vector<ll> h_all_twiddles;
    size_t total_twiddles = 0;
    for (int len = 2; len <= n; len <<= 1) {
        total_twiddles += len / 2;
    }
    h_all_twiddles.reserve(total_twiddles);

    ll root = power(3, (reducer.mod - 1) / n, reducer.mod);
    if (invert) {
        root = modInverse(root, reducer.mod);
    }
    
    for (int len = 2; len <= n; len <<= 1) {
        ll wlen_base = power(root, n / len, reducer.mod);
        ll w = 1;
        for (int j = 0; j < len / 2; j++) {
            h_all_twiddles.push_back(w);
            w = ((int128)w * wlen_base) % reducer.mod; 
        }
    }
    
    ll* d_all_twiddles;
    CUDA_CHECK(cudaMalloc((void**)&d_all_twiddles, total_twiddles * sizeof(ll)));
    CUDA_CHECK(cudaMemcpy(d_all_twiddles, h_all_twiddles.data(), total_twiddles * sizeof(ll), cudaMemcpyHostToDevice));

    int num_blocks_twid = (total_twiddles + threads_per_block - 1) / threads_per_block;
    to_mont_kernel<Reducer><<<num_blocks_twid, threads_per_block>>>(d_all_twiddles, total_twiddles, reducer);
    CUDA_CHECK(cudaGetLastError());

    size_t twiddle_offset = 0;
    for (int len = 2; len <= n; len <<= 1) {
        ll* d_twiddles_stage = d_all_twiddles + twiddle_offset;
        
        int num_threads = n / 2;
        int num_blocks = (num_threads + threads_per_block - 1) / threads_per_block;
        
        ntt_kernel_optimized<Reducer><<<num_blocks, threads_per_block>>>(d_a, d_twiddles_stage, len, n, reducer);
        CUDA_CHECK(cudaGetLastError());
        
        twiddle_offset += len / 2;
    }

    CUDA_CHECK(cudaFree(d_all_twiddles));
}

std::vector<ll> multiply_ntt_gpu(
    std::vector<ll>& poly1, std::vector<ll>& poly2, 
    ll mod, ll primitive_root, const std::string& method) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    if (n1 == 0 || n2 == 0) return {};
    
    int target_len = n1 + n2 - 1;
    int n = 1;
    while (n < target_len) n <<= 1;
    
    poly1.resize(n);
    poly2.resize(n);
    
    bit_reverse_copy(poly1);
    bit_reverse_copy(poly2);

    ll *d_p1, *d_p2;
    CUDA_CHECK(cudaMalloc((void**)&d_p1, n * sizeof(ll)));
    CUDA_CHECK(cudaMalloc((void**)&d_p2, n * sizeof(ll)));
    CUDA_CHECK(cudaMemcpy(d_p1, poly1.data(), n * sizeof(ll), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_p2, poly2.data(), n * sizeof(ll), cudaMemcpyHostToDevice));

    int threads = 256;
    int blocks = (n + threads - 1) / threads;

    if (method == "barrett") {
        BarrettReducer br(mod);
        ntt_gpu_optimized<BarrettReducer>(d_p1, n, false, br);
        ntt_gpu_optimized<BarrettReducer>(d_p2, n, false, br);
        pointwise_mult_kernel_optimized<BarrettReducer><<<blocks, threads>>>(d_p1, d_p1, d_p2, n, br);
        ntt_gpu_optimized<BarrettReducer>(d_p1, n, true, br);
        ll n_inv = modInverse(n, mod);
        final_scaling_kernel_optimized<BarrettReducer><<<blocks, threads>>>(d_p1, n, n_inv, br);
    } else if (method == "montgomery") {
        MontgomeryReducer mr(mod);
        
        to_mont_kernel<MontgomeryReducer><<<blocks, threads>>>(d_p1, n, mr);
        to_mont_kernel<MontgomeryReducer><<<blocks, threads>>>(d_p2, n, mr);

        ntt_gpu_montgomery(d_p1, n, false, mr);
        ntt_gpu_montgomery(d_p2, n, false, mr);

        pointwise_mult_kernel_optimized<MontgomeryReducer><<<blocks, threads>>>(d_p1, d_p1, d_p2, n, mr);
        
        ntt_gpu_montgomery(d_p1, n, true, mr);
        
        ll n_inv = modInverse(n, mod);
        ll n_inv_mont = mr.to_mont(n_inv);
        final_scaling_kernel_optimized<MontgomeryReducer><<<blocks, threads>>>(d_p1, n, n_inv_mont, mr);

        from_mont_kernel<MontgomeryReducer><<<blocks, threads>>>(d_p1, n, mr);

    } else { 
        ntt_gpu_naive(d_p1, n, false, mod, primitive_root);
        ntt_gpu_naive(d_p2, n, false, mod, primitive_root);
        pointwise_mult_kernel<<<blocks, threads>>>(d_p1, d_p1, d_p2, n, mod);
        ntt_gpu_naive(d_p1, n, true, mod, primitive_root);
        ll n_inv = modInverse(n, mod);
        final_scaling_kernel<<<blocks, threads>>>(d_p1, n, n_inv, mod);
    }

    std::vector<ll> result(n);
    CUDA_CHECK(cudaMemcpy(result.data(), d_p1, n * sizeof(ll), cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaFree(d_p1));
    CUDA_CHECK(cudaFree(d_p2));

    result.resize(target_len);
    return result;
}

int main() {
    std::vector<ll> p1, p2;
    ll mod = 998244353;
    ll primitive_root = 3;

    read_input(p1, p2, "input.txt");

    if (p1.empty() || p2.empty()) {
        std::cerr << "Error: Input vectors are empty. Check input.txt." << std::endl;
        return 1;
    }
    std::cout << "Read " << p1.size() << " coefficients for each polynomial." << std::endl;

    auto start_serial = std::chrono::high_resolution_clock::now();
    std::vector<ll> res_serial = multiply_ntt_serial(p1, p2, mod, primitive_root);
    auto end_serial = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> serial_time = end_serial - start_serial;
    std::cout << "--- Performance Results ---" << std::endl;
    std::cout << "CPU Serial time: " << serial_time.count() << " ms" << std::endl;
    write_output("output_serial.txt", res_serial);

    int n = 1;
    while (n < p1.size() + p2.size()) n <<= 1;
    
    std::vector<ll> h_p1 = p1;
    std::vector<ll> h_p2 = p2;
    h_p1.resize(n);
    h_p2.resize(n);
    bit_reverse_copy(h_p1);
    bit_reverse_copy(h_p2);

    ll *d_p1, *d_p2, *d_res;
    CUDA_CHECK(cudaMalloc((void**)&d_p1, n * sizeof(ll)));
    CUDA_CHECK(cudaMalloc((void**)&d_p2, n * sizeof(ll)));
    CUDA_CHECK(cudaMalloc((void**)&d_res, n * sizeof(ll)));

    CUDA_CHECK(cudaMemcpy(d_p1, h_p1.data(), n * sizeof(ll), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_p2, h_p2.data(), n * sizeof(ll), cudaMemcpyHostToDevice));

    int threads_per_block = 256;
    int num_blocks = (n + threads_per_block - 1) / threads_per_block;

    std::vector<std::string> methods = {"naive", "barrett", "montgomery"};
    for (const auto& method : methods) {
        cudaEvent_t start, stop;
        CUDA_CHECK(cudaEventCreate(&start));
        CUDA_CHECK(cudaEventCreate(&stop));

        CUDA_CHECK(cudaMemcpy(d_p1, h_p1.data(), n * sizeof(ll), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_p2, h_p2.data(), n * sizeof(ll), cudaMemcpyHostToDevice));

        CUDA_CHECK(cudaEventRecord(start));

        if (method == "naive") {
            ntt_gpu_naive(d_p1, n, false, mod, primitive_root);
            ntt_gpu_naive(d_p2, n, false, mod, primitive_root);
            pointwise_mult_kernel<<<num_blocks, threads_per_block>>>(d_res, d_p1, d_p2, n, mod);
            ntt_gpu_naive(d_res, n, true, mod, primitive_root);
            ll n_inv = modInverse(n, mod);
            final_scaling_kernel<<<num_blocks, threads_per_block>>>(d_res, n, n_inv, mod);
        } else if (method == "barrett") {
            BarrettReducer br(mod);
            ntt_gpu_optimized<BarrettReducer>(d_p1, n, false, br);
            ntt_gpu_optimized<BarrettReducer>(d_p2, n, false, br);
            pointwise_mult_kernel_optimized<BarrettReducer><<<num_blocks, threads_per_block>>>(d_res, d_p1, d_p2, n, br);
            ntt_gpu_optimized<BarrettReducer>(d_res, n, true, br);
            ll n_inv = modInverse(n, mod);
            final_scaling_kernel_optimized<BarrettReducer><<<num_blocks, threads_per_block>>>(d_res, n, n_inv, br);
        } else if (method == "montgomery") {
            MontgomeryReducer mr(mod);
            to_mont_kernel<MontgomeryReducer><<<num_blocks, threads_per_block>>>(d_p1, n, mr);
            to_mont_kernel<MontgomeryReducer><<<num_blocks, threads_per_block>>>(d_p2, n, mr);
            ntt_gpu_montgomery<MontgomeryReducer>(d_p1, n, false, mr);
            ntt_gpu_montgomery<MontgomeryReducer>(d_p2, n, false, mr);
            pointwise_mult_kernel_optimized<MontgomeryReducer><<<num_blocks, threads_per_block>>>(d_res, d_p1, d_p2, n, mr);
            ntt_gpu_montgomery<MontgomeryReducer>(d_res, n, true, mr);
            ll n_inv = modInverse(n, mod);
            ll n_inv_mont = mr.to_mont(n_inv);
            final_scaling_kernel_optimized<MontgomeryReducer><<<num_blocks, threads_per_block>>>(d_res, n, n_inv_mont, mr);
            from_mont_kernel<MontgomeryReducer><<<num_blocks, threads_per_block>>>(d_res, n, mr);
        }

        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK(cudaEventSynchronize(stop));
        
        float gpu_time_ms = 0;
        CUDA_CHECK(cudaEventElapsedTime(&gpu_time_ms, start, stop));

        std::vector<ll> res_gpu(n);
        CUDA_CHECK(cudaMemcpy(res_gpu.data(), d_res, n * sizeof(ll), cudaMemcpyDeviceToHost));
        res_gpu.resize(res_serial.size()); 

        std::cout << "GPU " << method << " kernel time: " << gpu_time_ms << " ms" << std::endl;
        std::cout << "  - Speedup vs CPU: " << serial_time.count() / gpu_time_ms << "x" << std::endl;
        
        std::string out_filename = "output_" + method + ".txt";
        write_output(out_filename, res_gpu);

        bool correct = (res_serial.size() == res_gpu.size());
        if(correct) {
            for(size_t i = 0; i < res_serial.size(); ++i) {
                if (res_serial[i] != res_gpu[i]) {
                    correct = false;
                    break;
                }
            }
        }
        std::cout << "  - Verification: " << (correct ? "PASSED" : "FAILED") << std::endl;

        CUDA_CHECK(cudaEventDestroy(start));
        CUDA_CHECK(cudaEventDestroy(stop));
    }

    CUDA_CHECK(cudaFree(d_p1));
    CUDA_CHECK(cudaFree(d_p2));
    CUDA_CHECK(cudaFree(d_res));

    return 0;
} 