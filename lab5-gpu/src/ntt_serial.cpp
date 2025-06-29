#include "common.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

typedef long long ll;

// ====================================================================================
// I/O Functions (implementations)
// ====================================================================================

void read_input(std::vector<ll>& p1, std::vector<ll>& p2, const std::string& filename) {
    std::cout << "Attempting to read file: " << filename << std::endl;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "  - Failed to open file." << std::endl;
        return;
    }
    std::string line;

    if (!std::getline(infile, line)) {
        std::cerr << "  - File is empty or could not read first line." << std::endl;
        return; 
    }
    std::cout << "  - Skipped metadata line: " << line << std::endl;

    if (std::getline(infile, line)) {
        std::stringstream ss(line);
        ll val;
        while (ss >> val) {
            p1.push_back(val);
        }
        std::cout << "  - Read " << p1.size() << " coefficients for p1." << std::endl;
    }

    if (std::getline(infile, line)) {
        std::stringstream ss(line);
        ll val;
        while (ss >> val) {
            p2.push_back(val);
        }
        std::cout << "  - Read " << p2.size() << " coefficients for p2." << std::endl;
    }
    std::cout << "Finished reading file." << std::endl;
}

void write_output(const std::string& filename, const std::vector<ll>& data) {
    std::ofstream outfile(filename);
    if (!outfile) {
        throw std::runtime_error("Cannot open output file: " + filename);
    }
    for (size_t i = 0; i < data.size(); ++i) {
        outfile << data[i] << (i == data.size() - 1 ? "" : " ");
    }
    outfile << std::endl;
}

// ====================================================================================
// Serial NTT Implementation (CPU Baseline)
// ====================================================================================

void ntt_serial(std::vector<ll>& a, bool invert, ll mod, ll primitive_root) {
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
        ll wlen = power(primitive_root, (mod - 1) / len, mod);
        if (invert)
            wlen = modInverse(wlen, mod);
        for (int i = 0; i < n; i += len) {
            ll w = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j];
                ll v = (static_cast<int128>(a[i + j + len / 2]) * w) % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = (static_cast<int128>(w) * wlen) % mod;
            }
        }
    }
}

std::vector<ll> multiply_ntt_serial(std::vector<ll> p1, std::vector<ll> p2, ll mod, ll primitive_root) {
    int n = 1;
    while (n < p1.size() + p2.size()) n <<= 1;
    p1.resize(n);
    p2.resize(n);

    ntt_serial(p1, false, mod, primitive_root);
    ntt_serial(p2, false, mod, primitive_root);

    std::vector<ll> res(n);
    for (int i = 0; i < n; i++)
        res[i] = (static_cast<int128>(p1[i]) * p2[i]) % mod;

    ntt_serial(res, true, mod, primitive_root);

    ll n_inv = modInverse(n, mod);
    for (int i = 0; i < n; i++)
        res[i] = (static_cast<int128>(res[i]) * n_inv) % mod;
    
    return res;
} 