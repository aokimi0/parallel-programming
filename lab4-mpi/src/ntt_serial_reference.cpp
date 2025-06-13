#include <iostream>
#include <vector>
#include <algorithm>

typedef long long ll;

ll power(ll base, ll exp, ll mod) {
    ll result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) {
            result = ((__int128)result * base) % mod;
        }
        base = ((__int128)base * base) % mod;
        exp >>= 1;
    }
    return result;
}

ll extended_gcd(ll a, ll b, ll& x, ll& y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }
    ll x1, y1;
    ll gcd = extended_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return gcd;
}

ll modInverse(ll n, ll mod) {
    ll x, y;
    ll gcd = extended_gcd(n, mod, x, y);
    if (gcd != 1) {
        throw std::runtime_error("Modular inverse does not exist");
    }
    return (x % mod + mod) % mod;
}

void bit_reverse_copy(std::vector<ll>& a) {
    int n = a.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }
}

void ntt_serial(std::vector<ll>& a, bool invert, ll mod, ll root) {
    int n = a.size();
    if (n <= 1) return;

    bit_reverse_copy(a);

    for (int len = 2; len <= n; len <<= 1) {
        ll wlen = power(root, (mod - 1) / len, mod);
        if (invert) {
            wlen = modInverse(wlen, mod);
        }

        for (int i = 0; i < n; i += len) {
            ll w = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j];
                ll v = ((__int128)a[i + j + len / 2] * w) % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = ((__int128)w * wlen) % mod;
            }
        }
    }

    if (invert) {
        ll n_inv = modInverse(n, mod);
        for (ll& x : a) {
            x = ((__int128)x * n_inv) % mod;
        }
    }
}

std::vector<ll> multiply_ntt_serial(
    std::vector<ll> poly1, 
    std::vector<ll> poly2, 
    ll mod, 
    ll primitive_root) {
    
    int n1 = poly1.size();
    int n2 = poly2.size();
    if (n1 == 0 || n2 == 0) return {};
    
    int target_len = n1 + n2 - 1;
    int n = 1;
    while (n < target_len) n <<= 1;
    
    poly1.resize(n);
    poly2.resize(n);
    
    ntt_serial(poly1, false, mod, primitive_root);
    ntt_serial(poly2, false, mod, primitive_root);
    
    std::vector<ll> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = ((__int128)poly1[i] * poly2[i]) % mod;
    }
    
    ntt_serial(result, true, mod, primitive_root);
    
    result.resize(target_len);
    return result;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    int n_coeffs;
    ll mod_val;
    
    if (!(std::cin >> n_coeffs >> mod_val)) {
        std::cerr << "Error: Failed to read input" << std::endl;
        return 1;
    }
    
    if (n_coeffs <= 0) {
        std::cerr << "Error: N_COEFFS must be positive" << std::endl;
        return 1;
    }
    
    std::vector<ll> p1(n_coeffs);
    std::vector<ll> p2(n_coeffs);
    
    for (int i = 0; i < n_coeffs; ++i) {
        if (!(std::cin >> p1[i])) {
            std::cerr << "Error: Failed to read polynomial A" << std::endl;
            return 1;
        }
    }
    for (int i = 0; i < n_coeffs; ++i) {
        if (!(std::cin >> p2[i])) {
            std::cerr << "Error: Failed to read polynomial B" << std::endl;
            return 1;
        }
    }
    
    ll primitive_root = 3;
    
    try {
        std::vector<ll> result = multiply_ntt_serial(p1, p2, mod_val, primitive_root);
        
        for (size_t i = 0; i < result.size(); ++i) {
            std::cout << result[i] << (i == result.size() - 1 ? "" : " ");
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 