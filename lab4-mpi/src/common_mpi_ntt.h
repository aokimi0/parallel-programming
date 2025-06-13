#ifndef COMMON_MPI_NTT_H
#define COMMON_MPI_NTT_H

#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <numeric>

typedef __int128 int128;
typedef long long ll;
typedef uint64_t u64;

extern const int128 MAX_INT128_VAL;

struct PrimeInfo {
    ll mod;
    ll root;
};

extern const std::vector<PrimeInfo> AVAILABLE_NTT_PRIMES;

// 基础数学函数
ll power(ll base, ll exp, ll mod);
ll modInverse(ll n, ll mod);
ll extended_gcd(ll a, ll b, ll& x, ll& y);
ll inv_exgcd(ll n, ll mod);

// Barrett规约结构和函数
struct BarrettReduction {
    ll mod;
    __int128 r;
    int k;
    
    BarrettReduction(ll m) : mod(m) {
        k = 64;
        r = ((__int128)1 << k) / mod;
    }
    
    ll reduce(__int128 x) const {
        __int128 q = (x * r) >> k;
        __int128 result = x - q * mod;
        if (result >= mod) result -= mod;
        if (result < 0) result += mod;
        return (ll)result;
    }
    
    ll multiply(ll a, ll b) const {
        return reduce((__int128)a * b);
    }
};

// Montgomery规约结构（备用）
struct MontgomeryReduction {
    ll mod;
    ll mod_inv;
    ll r;
    
    MontgomeryReduction(ll m);
    ll reduce(ll x) const;
    ll multiply(ll a, ll b) const;
};

// MPI并行NTT相关函数
void ntt_mpi_butterfly(std::vector<ll>& a, bool invert, ll mod, ll root, 
                      int rank, int size);
void ntt_sequential_barrett(std::vector<ll>& a, bool invert, ll mod, ll root);
void ntt_mpi_distributed(std::vector<ll>& a, bool invert, ll mod, ll root,
                         int rank, int size);
std::vector<ll> multiply_ntt_mpi(
    std::vector<ll> poly1, 
    std::vector<ll> poly2, 
    ll mod, 
    ll primitive_root,
    int rank,
    int size
);

// MPI蝶形运算并行
struct MPIButterflyData {
    std::vector<ll>* data;
    ll mod;
    ll wlen;
    int len;
    int start_block;
    int end_block;
    BarrettReduction* barrett;
};

// 位反转置换
void bit_reverse_copy(std::vector<ll>& a);

// MPI辅助函数
void distribute_data(std::vector<ll>& data, std::vector<ll>& local_data, 
                    int n, int rank, int size);
void gather_data(std::vector<ll>& local_data, std::vector<ll>& data, 
                 int n, int rank, int size);

#endif 