#include <arm_neon.h>
#include <bits/stdc++.h>
using namespace std;

using u64 = uint64_t;
using u32 = uint32_t;

const u64 BIG_MOD = 7696582450348003ULL;
const u32 MODS[2] = {7340033, 104857601};

u32 mp(u32 a, u32 b, u32 mod) {
    u32 r0 = 1;
    while (b) {
        if (b & 1) r0 = u64(r0) * a % mod;
        a = u64(a) * a % mod;
        b >>= 1;
    }
    return r0;
}

u64 crt2(u64 r1, u64 r2, u64 m1, u64 m2) {
    u64 m1i2 = mp(m1, m2 - 2, m2);
    u64 y1 = r1;
    u64 y2 = ((r2 + m2 - y1 % m2) * m1i2) % m2;
    return (y1 + m1 * y2) % (m1 * m2);
}

class M {
public:
    static u32 m, g, i, r;
    static int l, w;
    static void sm(u32 mm, u32 gg) {
        m = mm;
        g = gg;
        w = 8 * sizeof(u32);
        i = mi(m);
        r = -u64(m) % m;
        l = __builtin_ctzll(m - 1);
    }
    static u32 mi(u32 n, int e = 6, u32 x = 1) {
        return e == 0 ? x : mi(n, e - 1, x * (2 - x * n));
    }
    M() : x(0) {}
    M(u32 n) : x(init(n)) {}
    static u32 modulus() { return m; }
    static u32 init(u32 w_) { return reduce(u64(w_) * r); }
    static u32 reduce(const u64 w_) {
        return u32(w_ >> w) + m - u32((u64(u32(w_) * i) * m) >> w);
    }
    static M om() {
        return M(g).qp((m - 1) >> l);
    }
    M &operator+=(const M &o) {
        x += o.x;
        return *this;
    }
    M &operator-=(const M &o) {
        x += 3 * m - o.x;
        return *this;
    }
    M &operator*=(const M &o) {
        x = reduce(u64(x) * o.x);
        return *this;
    }
    M operator+(const M &o) const {
        return M(*this) += o;
    }
    M operator-(const M &o) const {
        return M(*this) -= o;
    }
    M operator*(const M &o) const {
        return M(*this) *= o;
    }
    u32 v() const { return reduce(x) % m; }
    void set(u32 n) { x = n; }
    M qp(u32 e) const {
        M ret = M(1);
        for (M base = *this; e; e >>= 1, base *= base)
            if (e & 1) ret *= base;
        return ret;
    }
    M iv() const { return qp(m - 2); }
    alignas(4) u32 x;
};

u32 M::m = 104857601;
u32 M::g = 3;
u32 M::i = 0;
u32 M::r = 0;
int M::l = 0;
int M::w = 8 * sizeof(u32);

vector<M> a1, a2;

inline uint32x4_t montgomery_mul4(uint32x4_t x, uint32x4_t y) {
    uint64x2_t prod0 = vmull_u32(vget_low_u32(x), vget_low_u32(y));
    uint64x2_t prod1 = vmull_u32(vget_high_u32(x), vget_high_u32(y));
    uint64_t tmp[4];
    vst1q_u64(tmp, prod0);
    vst1q_u64(tmp+2, prod1);
    uint32_t res[4];
    for(int k=0;k<4;++k) {
        res[k] = M::reduce(tmp[k]);
    }
    return vld1q_u32(res);
}

void prep_root(vector<M> &rt, vector<M> &irt) {
    rt[0] = M::om();
    for (int p = 1; p < M::l; p++) rt[p] = rt[p - 1] * rt[p - 1];
    irt[0] = rt[0].iv();
    for (int p = 1; p < M::l; p++)
        irt[p] = irt[p - 1] * irt[p - 1];
}

void tfm(vector<M> &a, int n, const vector<M> &rt, const vector<M> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M::l;
    M o = M(1);
    M im_scalar = rt[lvv - 2];

    vector<M> d(64);
    d[0] = rt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * irt[lvv - 1 - p] * rt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * irt[1];

    if (ln & 1) {
        uint32x4_t m3_vec = vdupq_n_u32(3 * M::m);
        int half = n >> 1;
        for (int p = 0; p < half; p += 4) {
            if (p + 4 > half) {
                for(int k=p; k<half; ++k) {
                    M u = a[k], v = a[k + half];
                    a[k] = u + v;
                    a[k + half] = u - v;
                }
                break;
            }
            uint32x4_t u_vec = vld1q_u32((u32*)&a[p]);
            uint32x4_t v_vec = vld1q_u32((u32*)&a[p + half]);
            uint32x4_t add_res = vaddq_u32(u_vec, v_vec);
            uint32x4_t u_plus_3m = vaddq_u32(u_vec, m3_vec);
            uint32x4_t sub_res = vsubq_u32(u_plus_3m, v_vec);
            vst1q_u32((u32*)&a[p], add_res);
            vst1q_u32((u32*)&a[p + half], sub_res);
        }
    }

    uint32x4_t m3_vec = vdupq_n_u32(3 * M::m);

    for (int u = ln & ~1; u >= 2; u -= 2) {
        int v = 1 << u;
        int v4 = v >> 2;
        M w2_scalar = o;

        for (int p = 0; p < n; p += v) {
            M w1_scalar = w2_scalar * w2_scalar;
            M w3_scalar = w1_scalar * w2_scalar;

            M current_o = o;
            M current_w1 = w1_scalar;
            M current_w2 = w2_scalar;
            M current_w3 = w3_scalar;

            for (int q = p; q < p + v4; q += 4) {
                if (q + 4 > p + v4) {
                    for(int k=q; k < p + v4; ++k) {
                        M x0 = a[k + v4 * 0] * current_o;
                        M x1 = a[k + v4 * 1] * current_w2;
                        M x2 = a[k + v4 * 2] * current_w1;
                        M x3 = a[k + v4 * 3] * current_w3;
                        M t02p = x0 + x2, t13p = x1 + x3;
                        M t02m = x0 - x2, t13m = (x1 - x3) * im_scalar;
                        a[k + v4 * 0].x = (t02p + t13p).x;
                        a[k + v4 * 1].x = (t02p - t13p).x;
                        a[k + v4 * 2].x = (t02m + t13m).x;
                        a[k + v4 * 3].x = (t02m - t13m).x;
                    }
                    break;
                }
                uint32x4_t x0_vec = vld1q_u32((u32*)&a[q + v4 * 0]);
                uint32x4_t x1_vec = vld1q_u32((u32*)&a[q + v4 * 1]);
                uint32x4_t x2_vec = vld1q_u32((u32*)&a[q + v4 * 2]);
                uint32x4_t x3_vec = vld1q_u32((u32*)&a[q + v4 * 3]);
                uint32x4_t o_vec = vdupq_n_u32(current_o.x);
                uint32x4_t w1_vec = vdupq_n_u32(current_w1.x);
                uint32x4_t w2_vec = vdupq_n_u32(current_w2.x);
                uint32x4_t w3_vec = vdupq_n_u32(current_w3.x);
                uint32x4_t im_vec = vdupq_n_u32(im_scalar.x);
                x0_vec = montgomery_mul4(x0_vec, o_vec);
                x1_vec = montgomery_mul4(x1_vec, w2_vec);
                x2_vec = montgomery_mul4(x2_vec, w1_vec);
                x3_vec = montgomery_mul4(x3_vec, w3_vec);
                uint32x4_t t02p_vec = vaddq_u32(x0_vec, x2_vec);
                uint32x4_t t13p_vec = vaddq_u32(x1_vec, x3_vec);
                uint32x4_t t02m_vec = vsubq_u32(vaddq_u32(x0_vec, m3_vec), x2_vec);
                uint32x4_t t13m_base_vec = vsubq_u32(vaddq_u32(x1_vec, m3_vec), x3_vec);
                t13m_base_vec = montgomery_mul4(t13m_base_vec, im_vec);
                uint32x4_t res0_vec = vaddq_u32(t02p_vec, t13p_vec);
                uint32x4_t res1_vec = vsubq_u32(vaddq_u32(t02p_vec, m3_vec), t13p_vec);
                uint32x4_t res2_vec = vaddq_u32(t02m_vec, t13m_base_vec);
                uint32x4_t res3_vec = vsubq_u32(vaddq_u32(t02m_vec, m3_vec), t13m_base_vec);
                vst1q_u32((u32*)&a[q + v4 * 0], res0_vec);
                vst1q_u32((u32*)&a[q + v4 * 1], res1_vec);
                vst1q_u32((u32*)&a[q + v4 * 2], res2_vec);
                vst1q_u32((u32*)&a[q + v4 * 3], res3_vec);
            }
            w2_scalar *= d[__builtin_ctz(~(p >> u))];
        }
    }
}

void itfm(vector<M> &a, int n, const vector<M> &rt, const vector<M> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M::l;
    M o = M(1);
    M im_scalar = irt[lvv - 2];

    vector<M> d(64);
    d[0] = irt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * rt[lvv - 1 - p] * irt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * rt[1];

    uint32x4_t m3_vec = vdupq_n_u32(3 * M::m);

    for (int u = 2; u <= ln; u += 2) {
        int v = 1 << u;
        int v4 = v >> 2;
        M w2_scalar = o;

        for (int p = 0; p < n; p += v) {
            M w1_scalar = w2_scalar * w2_scalar;
            M w3_scalar = w1_scalar * w2_scalar;

            M current_o = o;
            M current_w1 = w1_scalar;
            M current_w2 = w2_scalar;
            M current_w3 = w3_scalar;

            for (int q = p; q < p + v4; q += 4) {
                if (q + 4 > p + v4) {
                    for(int k=q; k < p + v4; ++k) {
                        M x0 = a[k + v4 * 0];
                        M x1 = a[k + v4 * 1];
                        M x2 = a[k + v4 * 2];
                        M x3 = a[k + v4 * 3];
                        M t01p = x0 + x1, t23p = x2 + x3;
                        M t01m = x0 - x1, t23m = (x2 - x3) * im_scalar;
                        a[k + v4 * 0] = (t01p + t23p) * current_o;
                        a[k + v4 * 2] = (t01p - t23p) * current_w1;
                        a[k + v4 * 1] = (t01m + t23m) * current_w2;
                        a[k + v4 * 3] = (t01m - t23m) * current_w3;
                    }
                    break;
                }
                uint32x4_t x0_vec = vld1q_u32((u32*)&a[q + v4 * 0]);
                uint32x4_t x1_vec = vld1q_u32((u32*)&a[q + v4 * 1]);
                uint32x4_t x2_vec = vld1q_u32((u32*)&a[q + v4 * 2]);
                uint32x4_t x3_vec = vld1q_u32((u32*)&a[q + v4 * 3]);
                uint32x4_t o_vec = vdupq_n_u32(current_o.x);
                uint32x4_t w1_vec = vdupq_n_u32(current_w1.x);
                uint32x4_t w2_vec = vdupq_n_u32(current_w2.x);
                uint32x4_t w3_vec = vdupq_n_u32(current_w3.x);
                uint32x4_t im_vec = vdupq_n_u32(im_scalar.x);
                uint32x4_t t01p_vec = vaddq_u32(x0_vec, x1_vec);
                uint32x4_t t23p_vec = vaddq_u32(x2_vec, x3_vec);
                uint32x4_t t01m_vec = vsubq_u32(vaddq_u32(x0_vec, m3_vec), x1_vec);
                uint32x4_t t23m_base_vec = vsubq_u32(vaddq_u32(x2_vec, m3_vec), x3_vec);
                t23m_base_vec = montgomery_mul4(t23m_base_vec, im_vec);
                uint32x4_t r0_base_vec = vaddq_u32(t01p_vec, t23p_vec);
                uint32x4_t r2_base_vec = vsubq_u32(vaddq_u32(t01p_vec, m3_vec), t23p_vec);
                uint32x4_t r1_base_vec = vaddq_u32(t01m_vec, t23m_base_vec);
                uint32x4_t r3_base_vec = vsubq_u32(vaddq_u32(t01m_vec, m3_vec), t23m_base_vec);
                r0_base_vec = montgomery_mul4(r0_base_vec, o_vec);
                r1_base_vec = montgomery_mul4(r1_base_vec, w2_vec);
                r2_base_vec = montgomery_mul4(r2_base_vec, w1_vec);
                r3_base_vec = montgomery_mul4(r3_base_vec, w3_vec);
                vst1q_u32((u32*)&a[q + v4 * 0], r0_base_vec);
                vst1q_u32((u32*)&a[q + v4 * 1], r1_base_vec);
                vst1q_u32((u32*)&a[q + v4 * 2], r2_base_vec);
                vst1q_u32((u32*)&a[q + v4 * 3], r3_base_vec);
            }
            w2_scalar *= d[__builtin_ctz(~(p >> u))];
        }
    }

    if (ln & 1) {
        int half = n >> 1;
         for (int p = 0; p < half; p += 4) {
              if (p + 4 > half) {
                 for(int k=p; k<half; ++k) {
                     M u = a[k], v = a[k + half];
                     a[k] = u + v;
                     a[k + half] = u - v;
                 }
                 break;
              }
             uint32x4_t u_vec = vld1q_u32((u32*)&a[p]);
             uint32x4_t v_vec = vld1q_u32((u32*)&a[p + half]);
             uint32x4_t add_res = vaddq_u32(u_vec, v_vec);
             uint32x4_t u_plus_3m = vaddq_u32(u_vec, m3_vec);
             uint32x4_t sub_res = vsubq_u32(u_plus_3m, v_vec);
             vst1q_u32((u32*)&a[p], add_res);
             vst1q_u32((u32*)&a[p + half], sub_res);
         }
    }
}

void cv(vector<M> &a1, int s1, vector<M> &a2, int s2, bool cyc = false) {
    int s = cyc ? std::max(s1, s2) : s1 + s2 - 1;
    int sz = 1 << (31 - __builtin_clz(2 * s - 1));
    assert(sz <= (u64(1) << M::l));
    vector<M> rt(64), irt(64);
    prep_root(rt, irt);
    a1.resize(sz);
    for (int i = s1; i < (int)a1.size(); ++i) a1[i] = M(0);
    tfm(a1, sz, rt, irt);
    M iv = M(sz).iv();
    if (&a1 == &a2 && s1 == s2) {
        for (int p = 0; p < sz; p++) a1[p] *= a1[p] * iv;
    } else {
        a2.resize(sz);
        for (int i = s2; i < (int)a2.size(); ++i) a2[i] = M(0);
        tfm(a2, sz, rt, irt);
        for (int p = 0; p < sz; p++) a1[p] *= a2[p] * iv;
    }
    itfm(a1, sz, rt, irt);
}

class M64 {
public:
    static u64 m, g;
    static int l;
    static void sm(u64 mm, u64 gg) {
        m = mm;
        g = gg;
        l = __builtin_ctzll(m - 1);
    }
    M64() : x(0) {}
    M64(u64 n) : x(n % m) {}
    static u64 modulus() { return m; }
    static M64 om() { return M64(g).qp((m - 1) >> l); }
    M64 &operator+=(const M64 &o) { x += o.x; if (x >= m) x -= m; return *this; }
    M64 &operator-=(const M64 &o) { x += m - o.x; if (x >= m) x -= m; return *this; }
    M64 &operator*=(const M64 &o) { x = (__uint128_t)x * o.x % m; return *this; }
    M64 operator+(const M64 &o) const { return M64(*this) += o; }
    M64 operator-(const M64 &o) const { return M64(*this) -= o; }
    M64 operator*(const M64 &o) const { return M64(*this) *= o; }
    u64 v() const { return x; }
    void set(u64 n) { x = n % m; }
    M64 qp(u64 e) const {
        M64 ret = M64(1);
        for (M64 base = *this; e; e >>= 1, base *= base)
            if (e & 1) ret *= base;
        return ret;
    }
    M64 iv() const { return qp(m - 2); }
    u64 x;
};
u64 M64::m = 104857601;
u64 M64::g = 3;
int M64::l = 0;

void prep_root64(vector<M64> &rt, vector<M64> &irt) {
    rt[0] = M64::om();
    for (int p = 1; p < M64::l; p++) rt[p] = rt[p - 1] * rt[p - 1];
    irt[0] = rt[0].iv();
    for (int p = 1; p < M64::l; p++)
        irt[p] = irt[p - 1] * irt[p - 1];
}

void tfm64(vector<M64> &a1, int n, const vector<M64> &rt, const vector<M64> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M64::l;
    M64 o = M64(1), im = rt[lvv - 2];
    vector<M64> d(64);
    d[0] = rt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * irt[lvv - 1 - p] * rt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * irt[1];
    if (ln & 1) {
        for (int p = 0; p < nhh; p++) {
            M64 u = a1[p], v = a1[p + nhh];
            a1[p] = u + v;
            a1[p + nhh] = u - v;
        }
    }
    for (int u = ln & ~1; u >= 2; u -= 2) {
        int v = 1 << u, v4 = v >> 2;
        M64 w2 = o;
        for (int p = 0; p < n; p += v) {
            M64 w1 = w2 * w2, w3 = w1 * w2;
            for (int q = p; q < p + v4; ++q) {
                M64 x0 = a1[q + v4 * 0] * o, x1 = a1[q + v4 * 1] * w2;
                M64 x2 = a1[q + v4 * 2] * w1, x3 = a1[q + v4 * 3] * w3;
                M64 t02p = x0 + x2, t13p = x1 + x3;
                M64 t02m = x0 - x2, t13m = (x1 - x3) * im;
                a1[q + v4 * 0] = t02p + t13p;
                a1[q + v4 * 1] = t02p - t13p;
                a1[q + v4 * 2] = t02m + t13m;
                a1[q + v4 * 3] = t02m - t13m;
            }
            w2 *= d[__builtin_ctz(~(p >> u))];
        }
    }
}

void itfm64(vector<M64> &a1, int n, const vector<M64> &rt, const vector<M64> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M64::l;
    M64 o = M64(1), im = irt[lvv - 2];
    vector<M64> d(64);
    d[0] = irt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * rt[lvv - 1 - p] * irt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * rt[1];
    for (int u = 2; u <= ln; u += 2) {
        int v = 1 << u, v4 = v >> 2;
        M64 w2 = o;
        for (int p = 0; p < n; p += v) {
            M64 w1 = w2 * w2, w3 = w1 * w2;
            for (int q = p; q < p + v4; ++q) {
                M64 x0 = a1[q + v4 * 0], x1 = a1[q + v4 * 1];
                M64 x2 = a1[q + v4 * 2], x3 = a1[q + v4 * 3];
                M64 t01p = x0 + x1, t23p = x2 + x3;
                M64 t01m = x0 - x1, t23m = (x2 - x3) * im;
                a1[q + v4 * 0] = (t01p + t23p) * o;
                a1[q + v4 * 2] = (t01p - t23p) * w1;
                a1[q + v4 * 1] = (t01m + t23m) * w2;
                a1[q + v4 * 3] = (t01m - t23m) * w3;
            }
            w2 *= d[__builtin_ctz(~(p >> u))];
        }
    }
    if (ln & 1) {
        for (int p = 0; p < nhh; p++) {
            M64 u = a1[p], v = a1[p + nhh];
            a1[p] = u + v;
            a1[p + nhh] = u - v;
        }
    }
}

void cv64(vector<M64> &a1, int s1, vector<M64> &a2, int s2, bool cyc = false) {
    int s = cyc ? std::max(s1, s2) : s1 + s2 - 1;
    int sz = 1 << (31 - __builtin_clz(2 * s - 1));
    assert(sz <= (u64(1) << M64::l));
    vector<M64> rt(64), irt(64);
    prep_root64(rt, irt);
    a1.resize(sz);
    for (int i = s1; i < (int)a1.size(); ++i) a1[i] = 0;
    tfm64(a1, sz, rt, irt);
    M64 iv = M64(sz).iv();
    if (&a1 == &a2 && s1 == s2) {
        for (int p = 0; p < sz; p++) a1[p] *= a1[p] * iv;
    } else {
        a2.resize(sz);
        for (int i = s2; i < (int)a2.size(); ++i) a2[i] = 0;
        tfm64(a2, sz, rt, irt);
        for (int p = 0; p < sz; p++) a1[p] *= a2[p] * iv;
    }
    itfm64(a1, sz, rt, irt);
}

int main() {
    int n;
    u64 mod;
    cin >> n >> mod;
    a1.resize(n + 1);
    a2.resize(n + 1);
    if (mod == BIG_MOD) {
        vector<u64> a3(n + 1), b3(n + 1);
        for (int p = 0; p <= n; p++) cin >> a3[p];
        for (int p = 0; p <= n; p++) cin >> b3[p];
        int l = 2 * n + 1;
        vector<vector<u64>> rr(2, vector<u64>(l));
        for (int z = 0; z < 2; ++z) {
            M::sm(MODS[z], 3);
            for (int p = 0; p <= n; p++) a1[p] = a3[p];
            for (int p = 0; p <= n; p++) a2[p] = b3[p];
            cv(a1, n + 1, a2, n + 1);
            for (int p = 0; p < l; ++p) rr[z][p] = a1[p].v();
        }
        for (int p = 0; p < l; ++p) {
            u64 ans = crt2(rr[0][p], rr[1][p], MODS[0], MODS[1]) % mod;
            cout << ans << (p + 1 == l ? '\n' : ' ');
        }
    } else {
        if (mod > u32(-1)) {
            vector<u64> a3(n + 1), b3(n + 1);
            for (int p = 0; p <= n; p++) cin >> a3[p];
            for (int p = 0; p <= n; p++) cin >> b3[p];
            vector<M64> aa(n + 1), bb(n + 1);
            M64::sm(mod, 3);
            for (int p = 0; p <= n; p++) aa[p] = a3[p];
            for (int p = 0; p <= n; p++) bb[p] = b3[p];
            cv64(aa, n + 1, bb, n + 1);
            for (int p = 0; p <= 2 * n; p++) cout << aa[p].v() << (p == 2 * n ? '\n' : ' ');
            return 0;
        }
        M::sm(mod, 3);
        for (int p = 0, y; p <= n; p++) {
            cin >> y;
            a1[p] = y;
        }
        for (int p = 0, y; p <= n; p++) {
            cin >> y;
            a2[p] = y;
        }
        cv(a1, n + 1, a2, n + 1);
        for (int p = 0; p <= 2 * n; p++) cout << a1[p].v() << (p == 2 * n ? '\n' : ' ');
    }
    return 0;
} 