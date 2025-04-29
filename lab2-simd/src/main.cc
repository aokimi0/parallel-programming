#include <bits/stdc++.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sys/time.h>
#include <omp.h>
#include <cstdint>

using namespace std;

using u64 = uint64_t;
using u32 = uint32_t;

u32 mp(u32 a, u32 b, u32 mod) {
    u32 r0 = 1;
    while (b) {
        if (b & 1) r0 = u64(r0) * a % mod;
        a = u64(a) * a % mod;
        b >>= 1;
    }
    return r0;
}

template<typename T>
class M {
public:
    static T m, g, i, r;
    static int l, w;
    static void sm(T mm, T gg) {
        m = mm;
        g = gg;
        w = 8 * sizeof(T);
        i = mi(m);
        r = -u64(m) % m;
        l = __builtin_ctzll(m - 1);
    }
    static T mi(T n, int e = 6, T x = 1) {
        return e == 0 ? x : mi(n, e - 1, x * (2 - x * n));
    }
    M() : x(0) {}
    M(T n) : x(init(n)) {}
    static T modulus() { return m; }
    static T init(T w_) { return reduce(u64(w_) * r); }
    static T reduce(const u64 w_) {
        return T(w_ >> w) + m - T((u64(T(w_) * i) * m) >> w);
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
    T v() const { return reduce(x) % m; }
    void set(T n) { x = n; }
    M qp(T e) const {
        M ret = M(1);
        for (M base = *this; e; e >>= 1, base *= base)
            if (e & 1) ret *= base;
        return ret;
    }
    M iv() const { return qp(m - 2); }
    T x;
};

template<typename T> T M<T>::m = 104857601;
template<typename T> T M<T>::g = 3;
template<typename T> T M<T>::i = 0;
template<typename T> T M<T>::r = 0;
template<typename T> int M<T>::l = 0;
template<typename T> int M<T>::w = 8 * sizeof(T);

template<typename T>
void prep_root(vector<M<T>> &rt, vector<M<T>> &irt) {
    rt[0] = M<T>::om();
    for (int p = 1; p < M<T>::l; p++) rt[p] = rt[p - 1] * rt[p - 1];
    irt[0] = rt[0].iv();
    for (int p = 1; p < M<T>::l; p++)
        irt[p] = irt[p - 1] * irt[p - 1];
}

template<typename T>
void tfm(vector<M<T>> &a1, int n, const vector<M<T>> &rt, const vector<M<T>> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M<T>::l;
    M<T> o = M<T>(1), im = rt[lvv - 2];
    vector<M<T>> d(64);
    d[0] = rt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * irt[lvv - 1 - p] * rt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * irt[1];
    if (ln & 1) {
        for (int p = 0; p < nhh; p++) {
            M<T> u = a1[p], v = a1[p + nhh];
            a1[p] = u + v;
            a1[p + nhh] = u - v;
        }
    }
    for (int u = ln & ~1; u >= 2; u -= 2) {
        int v = 1 << u, v4 = v >> 2;
        M<T> w2 = o;
        for (int p = 0; p < n; p += v) {
            M<T> w1 = w2 * w2, w3 = w1 * w2;
            for (int q = p; q < p + v4; ++q) {
                M<T> x0 = a1[q + v4 * 0] * o, x1 = a1[q + v4 * 1] * w2;
                M<T> x2 = a1[q + v4 * 2] * w1, x3 = a1[q + v4 * 3] * w3;
                M<T> t02p = x0 + x2, t13p = x1 + x3;
                M<T> t02m = x0 - x2, t13m = (x1 - x3) * im;
                a1[q + v4 * 0] = t02p + t13p;
                a1[q + v4 * 1] = t02p - t13p;
                a1[q + v4 * 2] = t02m + t13m;
                a1[q + v4 * 3] = t02m - t13m;
            }
            w2 *= d[__builtin_ctz(~(p >> u))];
        }
    }
}

template<typename T>
void itfm(vector<M<T>> &a1, int n, const vector<M<T>> &rt, const vector<M<T>> &irt) {
    int ln = __builtin_ctz(n), nhh = n >> 1, lvv = M<T>::l;
    M<T> o = M<T>(1), im = irt[lvv - 2];
    vector<M<T>> d(64);
    d[0] = irt[lvv - 3];
    for (int p = 1; p < lvv - 2; p++)
        d[p] = d[p - 1] * rt[lvv - 1 - p] * irt[lvv - 3 - p];
    d[lvv - 2] = d[lvv - 3] * rt[1];
    for (int u = 2; u <= ln; u += 2) {
        int v = 1 << u, v4 = v >> 2;
        M<T> w2 = o;
        for (int p = 0; p < n; p += v) {
            M<T> w1 = w2 * w2, w3 = w1 * w2;
            for (int q = p; q < p + v4; ++q) {
                M<T> x0 = a1[q + v4 * 0], x1 = a1[q + v4 * 1];
                M<T> x2 = a1[q + v4 * 2], x3 = a1[q + v4 * 3];
                M<T> t01p = x0 + x1, t23p = x2 + x3;
                M<T> t01m = x0 - x1, t23m = (x2 - x3) * im;
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
            M<T> u = a1[p], v = a1[p + nhh];
            a1[p] = u + v;
            a1[p + nhh] = u - v;
        }
    }
}

template<typename T>
void cv(vector<M<T>> &a1, int s1, vector<M<T>> &a2, int s2, bool cyc = false) {
    int s = cyc ? std::max(s1, s2) : s1 + s2 - 1;
    int sz = 1 << (31 - __builtin_clz(2 * s - 1));
    assert(sz <= (u64(1) << M<T>::l));
    vector<M<T>> rt(64), irt(64);
    prep_root<T>(rt, irt);
    a1.resize(sz);
    for (int i = s1; i < (int)a1.size(); ++i) a1[i] = 0;
    tfm<T>(a1, sz, rt, irt);
    M<T> iv = M<T>(sz).iv();
    if (&a1 == &a2 && s1 == s2) {
        for (int p = 0; p < sz; p++) a1[p] *= a1[p] * iv;
    } else {
        a2.resize(sz);
        for (int i = s2; i < (int)a2.size(); ++i) a2[i] = 0;
        tfm<T>(a2, sz, rt, irt);
        for (int p = 0; p < sz; p++) a1[p] *= a2[p] * iv;
    }
    itfm<T>(a1, sz, rt, irt);
}

template<>
class M<uint64_t> {
public:
    static u64 m, g;
    static int l;
    static void sm(u64 mm, u64 gg) {
        m = mm;
        g = gg;
        l = __builtin_ctzll(m - 1);
    }
    M() : x(0) {}
    M(u64 n) : x(n % m) {}
    static u64 modulus() { return m; }
    static M om() { return M(g).qp((m - 1) >> l); }
    M &operator+=(const M &o) { x += o.x; if (x >= m) x -= m; return *this; }
    M &operator-=(const M &o) { x += m - o.x; if (x >= m) x -= m; return *this; }
    M &operator*=(const M &o) { x = (__uint128_t)x * o.x % m; return *this; }
    M operator+(const M &o) const { return M(*this) += o; }
    M operator-(const M &o) const { return M(*this) -= o; }
    M operator*(const M &o) const { return M(*this) *= o; }
    u64 v() const { return x; }
    void set(u64 n) { x = n % m; }
    M qp(u64 e) const {
        M ret = M(1);
        for (M base = *this; e; e >>= 1, base *= base)
            if (e & 1) ret *= base;
        return ret;
    }
    M iv() const { return qp(m - 2); }
    u64 x;
};
u64 M<uint64_t>::m = 104857601;
u64 M<uint64_t>::g = 3;
int M<uint64_t>::l = 0;

void fRead(u64 *a, u64 *b, int *n, u64 *p, int input_id){
    // 数据输入函数
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    char data_path[strin.size() + 1];
    std::copy(strin.begin(), strin.end(), data_path);
    data_path[strin.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    fin>>*n>>*p;
    for (int i = 0; i < *n; i++){
        fin>>a[i];
    }
    for (int i = 0; i < *n; i++){   
        fin>>b[i];
    }
}

void fCheck(u64 *ab, int n, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char data_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), data_path);
    data_path[strout.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    for (int i = 0; i < n * 2 - 1; i++){
        u64 x;
        fin>>x;
        if(x != ab[i]){
            std::cout<<"多项式乘法结果错误"<<std::endl;
            return;
        }
    }
    std::cout<<"多项式乘法结果正确"<<std::endl;
    return;
}

void fWrite(u64 *ab, int n, int input_id){
    std::string str1 = "files/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char output_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), output_path);
    output_path[strout.size()] = '\0';
    std::ofstream fout;
    fout.open(output_path, std::ios::out);
    for (int i = 0; i < n * 2 - 1; i++){
        fout<<ab[i]<<'\n';
    }
}

void ntt_multiply(u64 *a_arr, u64 *b_arr, u64 *ab, int n, u64 p) {
    if (p > u32(-1)) {
        M<u64>::sm(p, 3);
        vector<M<u64>> a1(n), a2(n);
        for (int i = 0; i < n; i++) a1[i] = a_arr[i];
        for (int i = 0; i < n; i++) a2[i] = b_arr[i];
        cv<u64>(a1, n, a2, n);
        for (int i = 0; i < 2 * n - 1; i++) ab[i] = a1[i].v();
    } else {
        M<u32>::sm(p, 3);
        vector<M<u32>> a1(n), a2(n);
        for (int i = 0; i < n; i++) a1[i] = a_arr[i];
        for (int i = 0; i < n; i++) a2[i] = b_arr[i];
        cv<u32>(a1, n, a2, n);
        for (int i = 0; i < 2 * n - 1; i++) ab[i] = a1[i].v();
    }
}

void poly_multiply(u64 *a, u64 *b, u64 *ab, int n, u64 p){
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < n; ++j){
            ab[i+j]=( (u64)a[i] * b[j] % p + ab[i+j]) % p;
        }
    }
}

u64 a[300000], b[300000], ab[300000];
int main(int argc, char *argv[])
{
    // 保证输入的所有模数的原根均为 3, 且模数都能表示为 a \times 4 ^ k + 1 的形式
    // 输入模数分别为 7340033 104857601 469762049 1337006139375617
    // 第四个模数超过了整型表示范围, 如果实现此模数意义下的多项式乘法需要修改框架
    // 对第四个模数的输入数据不做必要要求, 如果要自行探索大模数 NTT, 请在完成前三个模数的基础代码及优化后实现大模数 NTT
    // 输入文件共五个, 第一个输入文件 n = 4, 其余四个文件分别对应四个模数, n = 131072
    // 在实现快速数论变化前, 后四个测试样例运行时间较久, 推荐调试正确性时只使用输入文件 1
    int test_begin = 0;
    int test_end = 4;
    for(int i = test_begin; i <= test_end; ++i){
        long double ans = 0;
        int n_;
        u64 p_;
        fRead(a, b, &n_, &p_, i);
        memset(ab, 0, sizeof(ab));
        auto Start = std::chrono::high_resolution_clock::now();
        
        ntt_multiply(a, b, ab, n_, p_);
        
        auto End = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::ratio<1,1000>>elapsed = End - Start;
        ans += elapsed.count();
        fCheck(ab, n_, i);
        std::cout<<"average latency for n = "<<n_<<" p = "<<p_<<" : "<<ans<<" (us) "<<std::endl;
        // 可以使用 fWrite 函数将 ab 的输出结果打印到 files 文件夹下
        // 禁止使用 cout 一次性输出大量文件内容
        fWrite(ab, n_, i);
    }
    return 0;
}
