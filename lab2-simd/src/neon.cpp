#include <arm_neon.h>
#include <bits/stdc++.h>
using namespace std;

using u64 = uint64_t;
using u32 = uint32_t;

constexpr u32 PRIMITIVE_ROOT = 3;
const u32 MODS[2] = {7340033, 104857601};
const u64 BIG_MOD = 7696582450348003ULL;

u32 MOD;

template<typename T>
T modpow(T a, T b, T mod) {
    T res = 1;
    while (b) {
        if (b & 1) res = (unsigned __int128)res * a % mod;
        a = (unsigned __int128)a * a % mod;
        b >>= 1;
    }
    return res;
}

template<typename T>
void prepare_roots(vector<T>& roots, int n, T mod, T primitive_root, bool invert) {
    T g = modpow(primitive_root, (mod - 1) / n, mod);
    if (invert) g = modpow(g, mod - 2, mod);
    roots.resize(n);
    roots[0] = 1;
    for (int i = 1; i < n; ++i) {
        roots[i] = (unsigned __int128)roots[i - 1] * g % mod;
    }
}

template<typename T>
void ntt(vector<T>& a, bool invert, T mod, T primitive_root) {
    int n = a.size();
    vector<T> roots;
    prepare_roots(roots, n, mod, primitive_root, invert);
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) swap(a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = n / len;
        for (int i = 0; i < n; i += len) {
            for (int j = 0; j < half; ++j) {
                T u = a[i + j];
                T v = (unsigned __int128)a[i + j + half] * roots[step * j] % mod;
                T t = u + v;
                if (t >= mod) t -= mod;
                T t2 = u + mod - v;
                if (t2 >= mod) t2 -= mod;
                a[i + j] = t;
                a[i + j + half] = t2;
            }
        }
    }
    if (invert) {
        T inv_n = modpow((T)n, mod - 2, mod);
        for (int i = 0; i < n; ++i) {
            a[i] = (unsigned __int128)a[i] * inv_n % mod;
        }
    }
}

u32 modpow_u32(u32 a, u32 b, u32 mod) {
    u32 res = 1;
    while (b) {
        if (b & 1) res = u64(res) * a % mod;
        a = u64(a) * a % mod;
        b >>= 1;
    }
    return res;
}

void prepare_roots_simd(vector<u32>& roots, int n, bool invert) {
    u32 g = modpow_u32(PRIMITIVE_ROOT, (MOD - 1) / n, MOD);
    if (invert) g = modpow_u32(g, MOD - 2, MOD);
    roots.resize(n);
    roots[0] = 1;
    for (int i = 1; i < n; ++i) {
        roots[i] = u64(roots[i - 1]) * g % MOD;
    }
}

void ntt_simd(vector<u32>& a, bool invert) {
    int n = a.size();
    vector<u32> roots;
    prepare_roots_simd(roots, n, invert);
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) swap(a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        int half = len >> 1;
        int step = n / len;
        for (int i = 0; i < n; i += len) {
            int j = 0;
            for (; j + 7 < half; j += 8) {
                // NEON: 8个一组，分两次4个处理
                alignas(16) u32 w_arr1[4], w_arr2[4], u_arr1[4], u_arr2[4], v_arr1[4], v_arr2[4];
                for (int k = 0; k < 4; ++k) {
                    w_arr1[k] = roots[step * (j + k)];
                    u_arr1[k] = a[i + j + k];
                    v_arr1[k] = a[i + j + half + k];
                    w_arr2[k] = roots[step * (j + 4 + k)];
                    u_arr2[k] = a[i + j + 4 + k];
                    v_arr2[k] = a[i + j + half + 4 + k];
                }
                u32 mul_arr1[4], mul_arr2[4];
                for (int k = 0; k < 4; ++k) {
                    mul_arr1[k] = u64(v_arr1[k]) * w_arr1[k] % MOD;
                    mul_arr2[k] = u64(v_arr2[k]) * w_arr2[k] % MOD;
                }
                uint32x4_t u1 = vld1q_u32(u_arr1);
                uint32x4_t u2 = vld1q_u32(u_arr2);
                uint32x4_t mul1 = vld1q_u32(mul_arr1);
                uint32x4_t mul2 = vld1q_u32(mul_arr2);
                uint32x4_t modv = vdupq_n_u32(MOD);
                uint32x4_t t1 = vaddq_u32(u1, mul1);
                uint32x4_t t2 = vaddq_u32(u2, mul2);
                uint32x4_t t21 = vaddq_u32(vsubq_u32(u1, mul1), modv);
                uint32x4_t t22 = vaddq_u32(vsubq_u32(u2, mul2), modv);
                // t >= MOD
                uint32x4_t mask1 = vcgeq_u32(t1, modv);
                uint32x4_t mask2 = vcgeq_u32(t2, modv);
                t1 = vsubq_u32(t1, vandq_u32(mask1, modv));
                t2 = vsubq_u32(t2, vandq_u32(mask2, modv));
                uint32x4_t mask21 = vcgeq_u32(t21, modv);
                uint32x4_t mask22 = vcgeq_u32(t22, modv);
                t21 = vsubq_u32(t21, vandq_u32(mask21, modv));
                t22 = vsubq_u32(t22, vandq_u32(mask22, modv));
                vst1q_u32(&a[i + j], t1);
                vst1q_u32(&a[i + j + 4], t2);
                vst1q_u32(&a[i + j + half], t21);
                vst1q_u32(&a[i + j + half + 4], t22);
            }
            for (; j < half; ++j) {
                u32 u = a[i + j];
                u32 v = u64(a[i + j + half]) * roots[step * j] % MOD;
                u32 t = u + v;
                if (t >= MOD) t -= MOD;
                u32 t2 = u + MOD - v;
                if (t2 >= MOD) t2 -= MOD;
                a[i + j] = t;
                a[i + j + half] = t2;
            }
        }
    }
    if (invert) {
        u32 inv_n = modpow_u32(n, MOD - 2, MOD);
        int i = 0;
        for (; i + 7 < n; i += 8) {
            alignas(16) u32 x_arr1[4], x_arr2[4];
            for (int k = 0; k < 4; ++k) x_arr1[k] = a[i + k];
            for (int k = 0; k < 4; ++k) x_arr2[k] = a[i + 4 + k];
            for (int k = 0; k < 4; ++k) x_arr1[k] = u64(x_arr1[k]) * inv_n % MOD;
            for (int k = 0; k < 4; ++k) x_arr2[k] = u64(x_arr2[k]) * inv_n % MOD;
            vst1q_u32(&a[i], vld1q_u32(x_arr1));
            vst1q_u32(&a[i + 4], vld1q_u32(x_arr2));
        }
        for (; i < n; ++i) {
            a[i] = u64(a[i]) * inv_n % MOD;
        }
    }
}

template<typename T>
vector<T> convolve(const vector<T>& a, const vector<T>& b, T mod, T primitive_root) {
    int n = 1;
    while (n < int(a.size() + b.size() - 1)) n <<= 1;
    vector<T> fa(a.begin(), a.end()), fb(b.begin(), b.end());
    fa.resize(n); fb.resize(n);
    ntt(fa, false, mod, primitive_root);
    ntt(fb, false, mod, primitive_root);
    for (int i = 0; i < n; ++i) fa[i] = (unsigned __int128)fa[i] * fb[i] % mod;
    ntt(fa, true, mod, primitive_root);
    fa.resize(a.size() + b.size() - 1);
    return fa;
}

vector<u32> ntt_convolve_simd(const vector<u32>& a, const vector<u32>& b) {
    int n = 1;
    while (n < int(a.size() + b.size() - 1)) n <<= 1;
    vector<u32> fa(a.begin(), a.end()), fb(b.begin(), b.end());
    fa.resize(n); fb.resize(n);
    ntt_simd(fa, false);
    ntt_simd(fb, false);
    for (int i = 0; i < n; ++i) fa[i] = u64(fa[i]) * fb[i] % MOD;
    ntt_simd(fa, true);
    fa.resize(a.size() + b.size() - 1);
    return fa;
}

u64 crt2(u64 r1, u64 r2, u64 m1, u64 m2) {
    u64 m1_inv_m2 = modpow<u64>(m1, m2 - 2, m2);
    u64 x1 = r1;
    u64 x2 = ((r2 + m2 - x1 % m2) * m1_inv_m2) % m2;
    return (x1 + m1 * x2) % (m1 * m2);
}

int main() {
    int n;
    u64 mod;
    cin >> n >> mod;
    vector<u64> a(n + 1), b(n + 1);
    for (int i = 0; i <= n; ++i) cin >> a[i];
    for (int i = 0; i <= n; ++i) cin >> b[i];
    if (mod == BIG_MOD) {
        int len = 2 * n + 1;
        vector<vector<u64>> r(2, vector<u64>(len));
        for (int t = 0; t < 2; ++t) {
            MOD = MODS[t];
            vector<u32> aa(n + 1), bb(n + 1);
            for (int i = 0; i <= n; ++i) aa[i] = a[i];
            for (int i = 0; i <= n; ++i) bb[i] = b[i];
            vector<u32> res = ntt_convolve_simd(aa, bb);
            for (int i = 0; i < len; ++i) r[t][i] = res[i];
        }
        for (int i = 0; i < len; ++i) {
            u64 ans = crt2(r[0][i], r[1][i], MODS[0], MODS[1]) % mod;
            cout << ans << (i + 1 == len ? '\n' : ' ');
        }
    } else if (mod > u32(-1)) {
        u64 MOD = mod;
        vector<u64> aa(n + 1), bb(n + 1);
        for (int i = 0; i <= n; ++i) aa[i] = a[i];
        for (int i = 0; i <= n; ++i) bb[i] = b[i];
        vector<u64> c = convolve(aa, bb, MOD, (u64)PRIMITIVE_ROOT);
        for (int i = 0; i <= 2 * n; ++i) cout << c[i] << (i + 1 == 2 * n + 1 ? '\n' : ' ');
    } else {
        MOD = mod;
        vector<u32> aa(n + 1), bb(n + 1);
        for (int i = 0; i <= n; ++i) aa[i] = a[i];
        for (int i = 0; i <= n; ++i) bb[i] = b[i];
        vector<u32> c = ntt_convolve_simd(aa, bb);
        for (int i = 0; i <= 2 * n; ++i) cout << c[i] << (i + 1 == 2 * n + 1 ? '\n' : ' ');
    }
    return 0;
} 