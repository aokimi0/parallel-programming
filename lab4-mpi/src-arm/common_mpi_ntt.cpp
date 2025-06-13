#include "common_mpi_ntt.h"

const int128 MAX_INT128_VAL = (int128(1) << 126) - 1;

const std::vector<PrimeInfo> AVAILABLE_NTT_PRIMES = {
    {7340033, 3},
    {104857601, 3},
    {469762049, 3},
    {998244353, 3}
};

MontgomeryReduction::MontgomeryReduction(ll m) : mod(m) {
    r = 1;
    while (r < mod) r <<= 1;
    mod_inv = 1;
    ll t = mod;
    for (int i = 0; i < 63; i++) {
        if (t & 1) mod_inv *= 2 - mod * mod_inv;
        t >>= 1;
    }
}

ll MontgomeryReduction::reduce(ll x) const {
    __int128 temp = (__int128)x * mod_inv;
    ll q = (ll)temp;
    ll result = (((__int128)x - (__int128)q * mod) >> 64);
    return result >= mod ? result - mod : result;
}

ll MontgomeryReduction::multiply(ll a, ll b) const {
    return reduce((__int128)a * b);
}

ll power(ll base, ll exp, ll mod) {
    ll result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) {
            result = ((__int128)result * base) % mod;
        }
        base = ((__int128)base * base) % mod;
        exp >>= 1;
    }
    return result;
}

// 扩展欧几里得算法
ll extended_gcd(ll a, ll b, ll& x, ll& y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }
    ll x1, y1;
    ll gcd = extended_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return gcd;
}

// 模逆元
ll modInverse(ll n, ll mod) {
    ll x, y;
    ll gcd = extended_gcd(n, mod, x, y);
    if (gcd != 1) {
        throw std::runtime_error("Modular inverse does not exist");
    }
    return (x % mod + mod) % mod;
}

// 位反转置换
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

// Barrett规约优化的串行NTT实现
void ntt_sequential_barrett(std::vector<ll>& a, bool invert, ll mod, ll root) {
    int n = a.size();
    if (n <= 1) return;

    BarrettReduction barrett(mod);
    
    // 位反转置换
    bit_reverse_copy(a);

    // Cooley-Tukey NTT with Barrett reduction
    for (int len = 2; len <= n; len <<= 1) {
        ll wlen = power(root, (mod - 1) / len, mod);
        if (invert) {
            wlen = modInverse(wlen, mod);
        }

        for (int i = 0; i < n; i += len) {
            ll w = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j];
                ll v = barrett.multiply(a[i + j + len / 2], w);
                a[i + j] = (u + v >= mod) ? (u + v - mod) : (u + v);
                a[i + j + len / 2] = (u >= v) ? (u - v) : (u - v + mod);
                w = barrett.multiply(w, wlen);
            }
        }
    }

    if (invert) {
        ll n_inv = modInverse(n, mod);
        for (ll& x : a) {
            x = barrett.multiply(x, n_inv);
        }
    }
}

// MPI数据分发
void distribute_data(std::vector<ll>& data, std::vector<ll>& local_data, 
                    int n, int rank, int size) {
    int chunk_size = n / size;
    int remainder = n % size;
    
    int local_size = chunk_size + (rank < remainder ? 1 : 0);
    local_data.resize(local_size);
    
    int start = rank * chunk_size + std::min(rank, remainder);
    
    if (rank == 0) {
        // 主进程分发数据
        std::copy(data.begin() + start, data.begin() + start + local_size, 
                 local_data.begin());
        
        // 发送给其他进程
        for (int i = 1; i < size; i++) {
            int other_size = chunk_size + (i < remainder ? 1 : 0);
            int other_start = i * chunk_size + std::min(i, remainder);
            
            MPI_Send(&data[other_start], other_size, MPI_LONG_LONG, i, 0, MPI_COMM_WORLD);
        }
    } else {
        // 其他进程接收数据
        MPI_Recv(&local_data[0], local_size, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD, 
                MPI_STATUS_IGNORE);
    }
}

// MPI数据收集
void gather_data(std::vector<ll>& local_data, std::vector<ll>& data, 
                 int n, int rank, int size) {
    int chunk_size = n / size;
    int remainder = n % size;
    
    if (rank == 0) {
        // 计算主进程的起始位置
        int start = 0;
        
        // 复制主进程的数据
        std::copy(local_data.begin(), local_data.end(), data.begin() + start);
        
        // 接收其他进程的数据
        for (int i = 1; i < size; i++) {
            int other_size = chunk_size + (i < remainder ? 1 : 0);
            int other_start = i * chunk_size + std::min(i, remainder);
            
            MPI_Recv(&data[other_start], other_size, MPI_LONG_LONG, i, 0, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        // 其他进程发送数据
        MPI_Send(&local_data[0], local_data.size(), MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }
}

// MPI并行NTT实现
void ntt_mpi_butterfly(std::vector<ll>& a, bool invert, ll mod, ll root, 
                      int rank, int size) {
    if (rank == 0) {
        ntt_sequential_barrett(a, invert, mod, root);
    }
    MPI_Bcast(&a[0], a.size(), MPI_LONG_LONG, 0, MPI_COMM_WORLD);
}

void ntt_mpi_distributed(std::vector<ll>& a, bool invert, ll mod, ll root,
                         int rank, int size) {
    if (rank == 0) {
        ntt_sequential_barrett(a, invert, mod, root);
    }
    MPI_Bcast(&a[0], a.size(), MPI_LONG_LONG, 0, MPI_COMM_WORLD);
}

// MPI并行多项式乘法
std::vector<ll> multiply_ntt_mpi(
    std::vector<ll> poly1, 
    std::vector<ll> poly2, 
    ll mod, 
    ll primitive_root,
    int rank,
    int size) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    if (n1 == 0 || n2 == 0) return {};
    
    int target_len = n1 + n2 - 1;
    int n = 1;
    while (n < target_len) n <<= 1;
    
    poly1.resize(n);
    poly2.resize(n);
    
    ntt_mpi_butterfly(poly1, false, mod, primitive_root, rank, size);
    ntt_mpi_butterfly(poly2, false, mod, primitive_root, rank, size);
    
    BarrettReduction barrett(mod);
    int elements_per_proc = (n + size - 1) / size;
    int start_idx = rank * elements_per_proc;
    int end_idx = std::min(start_idx + elements_per_proc, n);
    
    std::vector<ll> result(n);
    for (int i = start_idx; i < end_idx; i++) {
        result[i] = barrett.multiply(poly1[i], poly2[i]);
    }
    
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, &result[0], elements_per_proc, MPI_LONG_LONG, MPI_COMM_WORLD);
    
    ntt_mpi_butterfly(result, true, mod, primitive_root, rank, size);
    
    result.resize(target_len);
    return result;
} 