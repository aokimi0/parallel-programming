#ifndef COMMON_CRT_NTT_H
#define COMMON_CRT_NTT_H

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

ll power(ll base, ll exp, ll mod);
ll modInverse(ll n, ll mod);
ll extended_gcd(ll a, ll b, ll& x, ll& y);
ll inv_exgcd(ll n, ll mod);

void ntt(std::vector<ll>& a, bool invert, ll mod, ll root);

std::vector<PrimeInfo> select_crt_moduli(int n_orig_coeffs, ll final_p_target);

#endif