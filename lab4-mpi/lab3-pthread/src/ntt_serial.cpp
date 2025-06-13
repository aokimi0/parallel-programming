#ifndef NTT_SERIAL_CPP_INCLUDED
#define NTT_SERIAL_CPP_INCLUDED

#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <stdexcept>
#include <fstream>
#include "common_crt_ntt.h"

void ntt_transform_serial_local(std::vector<long long>& a, bool invert_flag, long long mod, long long primitive_root) {
    int n = a.size();

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        long long wlen = power(primitive_root, (mod - 1) / len, mod);
        if (invert_flag) {
            wlen = modInverse(wlen, mod);
        }
        for (int i = 0; i < n; i += len) {
            long long w = 1;
            for (int j = 0; j < len / 2; j++) {
                long long u = a[i + j];
                long long v = ((__int128)a[i + j + len / 2] * w) % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = ((__int128)w * wlen) % mod;
            }
        }
    }

    if (invert_flag) {
        long long n_inv = modInverse(n, mod);
        for (long long& x : a) {
            x = ((__int128)x * n_inv) % mod;
        }
    }
}

std::vector<long long> multiply_ntt_serial_local(std::vector<long long> poly1, std::vector<long long> poly2, long long mod, long long primitive_root) {
    int n1 = poly1.size();
    int n2 = poly2.size();
    int n_fft = 1;
    if (n1 == 0 || n2 == 0) return {};
    int target_conv_len = n1 + n2 - 1;
    if (target_conv_len <= 0) target_conv_len = 1;

    while (n_fft < target_conv_len) {
        n_fft <<= 1;
    }
    if (n_fft == 0) n_fft = 1;


    poly1.resize(n_fft);
    poly2.resize(n_fft);

    ntt_transform_serial_local(poly1, false, mod, primitive_root);
    ntt_transform_serial_local(poly2, false, mod, primitive_root);

    std::vector<long long> result(n_fft);
    for (int i = 0; i < n_fft; i++) {
        result[i] = ((__int128)poly1[i] * poly2[i]) % mod;
    }

    ntt_transform_serial_local(result, true, mod, primitive_root);
    
    if (target_conv_len > 0) {
         result.resize(target_conv_len);
    } else {
        result.clear(); 
    }

    return result;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int n_coeffs;
    long long mod_val;
    
    if (!(std::cin >> n_coeffs >> mod_val)) {
        std::cerr << "Error: Failed to read N_COEFFS and MOD from stdin." << std::endl;
        return 1;
    }

    if (n_coeffs <= 0) {
        std::cerr << "Error: N_COEFFS must be positive. Got " << n_coeffs << std::endl;
        return 1;
    }

    std::vector<long long> p1(n_coeffs);
    std::vector<long long> p2(n_coeffs);

    for (int i = 0; i < n_coeffs; ++i) {
        if (!(std::cin >> p1[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial A." << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < n_coeffs; ++i) {
        if (!(std::cin >> p2[i])) {
            std::cerr << "Error: Failed to read coefficients for polynomial B." << std::endl;
            return 1;
        }
    }
    
    long long primitive_root = 3; 
    try {
        std::vector<long long> result = multiply_ntt_serial_local(p1, p2, mod_val, primitive_root);

        for (size_t i = 0; i < result.size(); ++i) {
            std::cout << result[i] << (i == result.size() - 1 ? "" : " ");
        }
        std::cout << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error in ntt_serial: " << e.what() << std::endl;
        return 1; // Exit with error code
    }

    return 0;
}
#endif