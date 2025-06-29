#include "common.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <immintrin.h> 

typedef long long ll;

struct MontgomeryReducer {
    ll mod;
    ll r;       
    ll r_inv;   
    ll mod_inv; 
    ll r2;      

    MontgomeryReducer(ll m) : mod(m) {
        r = 1LL << 63; 
        r_inv = power(r, mod - 2, mod);
        r2 = (static_cast<unsigned __int128>(r) * r) % mod;

        ll m_inv = 1, t = 0, x = r, y = mod;
        while (y) {
            ll q = x / y;
            std::swap(y, x %= y);
            std::swap(t, m_inv -= q * t);
        }
        mod_inv = (m_inv % r + r) % r;
    }

    ll to_mont(ll a) const {
        return (static_cast<unsigned __int128>(a) * r2) % mod;
    }

    ll from_mont(ll a_mont) const {
        return reduce(a_mont);
    }
    
    ll multiply(ll a_mont, ll b_mont) const {
        return reduce(static_cast<unsigned __int128>(a_mont) * b_mont);
    }

    ll reduce(unsigned __int128 t) const {
        ll m = static_cast<ll>(t) * mod_inv;
        ll res = (t + static_cast<unsigned __int128>(m) * mod) >> 63;
        return (res >= mod) ? (res - mod) : res;
    }
};

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

void ntt_simd(ll* a, int n, ll mod, bool invert) {
    for (int i = 0; i < n; i++) {
        int bit = n >> 1;
        for (int j = 0; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        ll wlen = power(3, (mod - 1) / len, mod);
        if (invert) wlen = power(wlen, mod - 2, mod);
        for (int i = 0; i < n; i += len) {
            ll w = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j];
                ll v = (a[i + j + len / 2] * w) % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = (w * wlen) % mod;
            }
        }
    }

    if (invert) {
        ll n_inv = power(n, mod - 2, mod);
        for (int i = 0; i < n; i++) {
            a[i] = (a[i] * n_inv) % mod;
        }
    }
}

void poly_mul_simd(const std::vector<ll>& poly1, const std::vector<ll>& poly2, std::vector<ll>& result, ll mod) {
    int deg1 = poly1.size() - 1;
    int deg2 = poly2.size() - 1;
    int n = 1;
    while (n <= deg1 + deg2) {
        n <<= 1;
    }

    std::vector<ll> p1(n, 0), p2(n, 0);
    for (int i = 0; i <= deg1; i++) p1[i] = poly1[i];
    for (int i = 0; i <= deg2; i++) p2[i] = poly2[i];

    ntt_simd(p1.data(), n, mod, false);
    ntt_simd(p2.data(), n, mod, false);

    result.resize(n);
    for (int i = 0; i < n; i++) {
        result[i] = (p1[i] * p2[i]) % mod;
    }

    ntt_simd(result.data(), n, mod, true);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <mod_id>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    int mod_id = std::stoi(argv[2]);

    ll mod = fSetMOD(mod_id);

    std::vector<ll> poly1, poly2;
    fRead(poly1, poly2, input_filename);

    std::vector<ll> result;
    auto start = std::chrono::high_resolution_clock::now();
    poly_mul_simd(poly1, poly2, result, mod);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "SIMD NTT execution time: " << duration.count() << " ms" << std::endl;
    
    if (fCheck(poly1, poly2, result, mod)) {
        std::cout << "Correct." << std::endl;
    } else {
        std::cout << "Incorrect." << std::endl;
    }
    
    return 0;
} 