#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <omp.h>
#include <cstdint>
#include <chrono> 

using u64 = uint64_t;
using u32 = uint32_t;

class M {
public:
    static u32 m, g, i, r;
    static int l, w;

    static void sm(u32 mm, u32 gg) {
        m = mm;
        g = gg;
        w = 8 * sizeof(u32);
        i = 0 - mi(m);
        r = -u64(m) % m;
        l = __builtin_ctzll(m - 1);
    }

    static u32 mi(u32 n, int e = 6, u32 x = 1) {
        return e == 0 ? x : mi(n, e - 1, x * (2 - x * n));
    }

    M() : x(0) {}
    M(u32 n) : x(init(n)) {}

    static u32 modulus() { return m; }
    static u32 init(u32 w_) { return reduce((u64)w_ * r); }
    static u32 reduce(const u64 w_) {
        u64 q = (u64)(u32)w_ * i;
        u64 res = (w_ + (u64)(u32)q * m) >> w;
        return (res < m) ? res : res - m;
    }

    static M om() { return M(g).qp((m - 1) >> l); }

    M& operator+=(const M& o) { x += o.x; if (x >= m) x -= m; return *this; }
    M& operator-=(const M& o) { x += m - o.x; if (x >= m) x -= m; return *this; }
    M& operator*=(const M& o) { x = reduce((u64)x * o.x); return *this; }
    M operator+(const M& o) const { return M(*this) += o; }
    M operator-(const M& o) const { return M(*this) -= o; }
    M operator*(const M& o) const { return M(*this) *= o; }
    
    u32 v() const { 
        return reduce(x);
    }

    M qp(u32 e) const {
        M ret = M(1);
        for (M base = *this; e; e >>= 1, base *= base)
            if (e & 1) ret *= base;
        return ret;
    }
    M iv() const { return qp(m - 2); }

    u32 x;
};

u32 M::m;
u32 M::g;
u32 M::i;
u32 M::r;
int M::l;
int M::w;

void ntt_montgomery_omp(std::vector<M>& a, bool invert, int num_threads) {
    int n = a.size();
    if (n == 0) return;
    omp_set_num_threads(num_threads);

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        M wlen = M(M::g).qp((M::m - 1) / len);
        if (invert) wlen = wlen.iv();
        
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i += len) {
            M w(1);
            for (int j = 0; j < len / 2; j++) {
                M u = a[i + j];
                M v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        M n_inv = M(n).iv();
        #pragma omp parallel for
        for (int i = 0; i < n; i++) {
            a[i] *= n_inv;
        }
    }
}

std::vector<u32> multiply_ntt(
    const std::vector<u32>& poly1_in, 
    const std::vector<u32>& poly2_in, 
    u32 mod, u32 root, int num_threads) {
    
    M::sm(mod, root);
    int n1 = poly1_in.size();
    int n2 = poly2_in.size();
    if (n1 == 0 || n2 == 0) return {};
    
    int n = 1;
    while (n < n1 + n2 - 1) n <<= 1;

    std::vector<M> poly1(n), poly2(n);
    for(int i=0; i<n1; ++i) poly1[i] = M(poly1_in[i]);
    for(int i=0; i<n2; ++i) poly2[i] = M(poly2_in[i]);

    ntt_montgomery_omp(poly1, false, num_threads);
    ntt_montgomery_omp(poly2, false, num_threads);

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        poly1[i] *= poly2[i];
    }

    ntt_montgomery_omp(poly1, true, num_threads);

    std::vector<u32> result(n1 + n2 - 1);
    for(size_t i = 0; i < result.size(); ++i) {
        result[i] = poly1[i].v();
    }
    return result;
}

void print_poly(const std::vector<u32>& p) {
    for (size_t i = 0; i < p.size(); ++i) {
        std::cout << p[i] << (i == p.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int num_threads = 1;
    if (argc > 1) {
        try { num_threads = std::stoi(argv[1]); }
        catch (const std::exception& e) { num_threads = 1; }
    }

    int n_coeffs; 
    u32 mod_val, root_val = 3; 
    
    std::cin >> n_coeffs >> mod_val;
    
    std::vector<u32> p1(n_coeffs), p2(n_coeffs); 
    for (int i = 0; i < n_coeffs; ++i) std::cin >> p1[i];
    for (int i = 0; i < n_coeffs; ++i) std::cin >> p2[i];
    
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = multiply_ntt(p1, p2, mod_val, root_val, num_threads);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> elapsed_ms = end_time - start_time;
        std::cerr << "Threads: " << num_threads << ", Time: " << elapsed_ms.count() << " ms" << std::endl;

        print_poly(result);
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 