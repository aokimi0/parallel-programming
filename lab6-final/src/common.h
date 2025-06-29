#pragma once

#ifndef __NVCC__
#define __host__
#define __device__
#endif

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <numeric>

#ifdef __GNUC__
using int128 = __int128;
#else
using int128 = long long; 
#endif

using ll = long long;

__host__ __device__ inline ll power(ll base, ll exp, ll mod) {
    ll res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (int128)res * base % mod;
        base = (int128)base * base % mod;
        exp /= 2;
    }
    return res;
}

__host__ __device__ inline ll modInverse(ll n, ll mod) {
    return power(n, mod - 2, mod);
}

__host__ __device__ inline ll extended_gcd_device(ll a, ll b, ll &x, ll &y) {
    if (a == 0) {
        x = 0; y = 1;
        return b;
    }
    ll x1, y1;
    ll gcd = extended_gcd_device(b % a, a, x1, y1);
    x = y1 - (b / a) * x1;
    y = x1;
    return gcd;
}

struct BarrettReducer {
    ll mod;
    unsigned __int128 mu;

    __host__ __device__ BarrettReducer(ll m) : mod(m), mu(0) {
        if (m != 0) {
            mu = (unsigned __int128)-1 / m + 1;
        }
    }

    __host__ __device__ ll multiply(ll a, ll b) const {
        if (mod == 0) return (ll)((unsigned __int128)a * b);
        unsigned __int128 prod = (unsigned __int128)a * b;
        unsigned __int128 q = (prod * mu) >> 64;
        ll r = (ll)(prod - q * mod);
        if (r < 0) r += mod;
        if (r >= mod) r -= mod;
        return r;
    }
};


struct MontgomeryReducer {
    ll mod;
    ll r_inv; 
    ll mod_prime; 
    static constexpr int R_BITS = 62;
    static constexpr ll R = 1LL << R_BITS;

    __host__ __device__ MontgomeryReducer(ll m = 0) : mod(m), r_inv(0), mod_prime(0) {
        if (mod == 0) return;
        
        ll y;
        extended_gcd_device(R, mod, r_inv, y);
        r_inv = (r_inv % mod + mod) % mod;
        
        mod_prime = 0;
        ll t_inv = 0;
        for (int i = 0; i < R_BITS; ++i) {
            if (!(t_inv & 1)) {
                t_inv += mod;
                mod_prime |= (1LL << i);
            }
            t_inv >>= 1;
        }
    }
    
    __host__ __device__ ll to_mont(ll a) const {
        return (int128(a) << R_BITS) % mod;
    }

    __host__ __device__ ll from_mont(ll a_mont) const {
        ll m = ((int128(a_mont) * mod_prime) & (R - 1));
        ll t = (a_mont + m * mod) >> R_BITS;
        if (t >= mod) return t - mod;
        return t;
    }
    
    __host__ __device__ ll multiply(ll a_mont, ll b_mont) const {
       int128 t = (int128)a_mont * b_mont;
        ll m = ((t & (R - 1)) * mod_prime) & (R - 1);
        ll u = (t + m * mod) >> R_BITS;
        if (u >= mod) return u - mod;
        return u;
    }
};

void read_input(std::vector<ll>& p1, std::vector<ll>& p2, const std::string& filename = "../input.txt");
void write_output(const std::string& filename, const std::vector<ll>& data);
