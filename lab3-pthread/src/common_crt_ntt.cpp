#include "common_crt_ntt.h"

// Definition of MAX_INT128_VAL
const int128 MAX_INT128_VAL = (((int128)1 << 126) - 1 + ((int128)1 << 126));

// Definition of AVAILABLE_NTT_PRIMES
const std::vector<PrimeInfo> AVAILABLE_NTT_PRIMES = {
    {1231453023109121LL, 3},  
    {263882790666241LL, 7},   
    {79164837199873LL, 5},    
    {39582418599937LL, 5},    
    {6597069766657LL, 5},     
    {2748779069441LL, 3},    
    {2061584302081LL, 7},    
    {206158430209LL, 22},
    {77309411329LL, 7},
    {75161927681LL, 3},
    {3221225473LL, 5},
    {2281701377LL, 3},
    {2013265921LL, 31},
    {1004535809LL, 3},
    {469762049LL, 3},
    {167772161LL, 3},
    {104857601LL, 3},
    {23068673LL, 3},
    {7340033LL, 3},
    {5767169LL, 3},
    {786433LL, 10},
    {65537LL, 3},
    {40961LL, 3},
    {12289LL, 11},
    {7681LL, 17},
    {257LL, 3},
    {193LL, 5},
    {97LL, 5},
    {17LL, 3}
};

ll power(ll base, ll exp, ll mod) {
    ll res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (int128)res * base % mod;
        base = (int128)base * base % mod;
        exp /= 2;
    }
    return res;
}

ll modInverse(ll n, ll mod) {
    return inv_exgcd(n, mod);
}

ll extended_gcd(ll a, ll b, ll& x, ll& y) {
    if (a == 0) {
        x = 0; y = 1; return b;
    }
    ll x1, y1;
    ll gcd = extended_gcd(b % a, a, x1, y1);
    x = y1 - (b / a) * x1;
    y = x1;
    return gcd;
}

ll inv_exgcd(ll n, ll mod) {
    ll x, y;
    ll g = extended_gcd(n, mod, x, y);
    if (g != 1) {
        throw std::runtime_error("Modular inverse does not exist (inv_exgcd).");
    }
    return (x % mod + mod) % mod;
}

void ntt(std::vector<ll>& a, bool invert, ll mod, ll root) {
    int n = a.size();
    if (n == 0) return;
    std::vector<int> rev(n);
    for(int i=0; i<n; ++i) {
        rev[i] = (rev[i>>1]>>1) | ((i&1) ? (n>>1) : 0);
        if (i < rev[i]) std::swap(a[i], a[rev[i]]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        ll wlen = power(root, (mod - 1) / len, mod);
        if (invert) wlen = modInverse(wlen, mod);
        for (int i = 0; i < n; i += len) {
            ll w = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j];
                ll v = (int128)a[i + j + len / 2] * w % mod;
                a[i + j] = (u + v) % mod;
                a[i + j + len / 2] = (u - v + mod) % mod;
                w = (int128)w * wlen % mod;
            }
        }
    }
    if (invert) {
        ll n_inv = modInverse(n, mod);
        for (ll& x : a) x = (int128)x * n_inv % mod;
    }
}

std::vector<PrimeInfo> select_crt_moduli(int n_orig_coeffs, ll final_p_target) {
    std::vector<PrimeInfo> candidate_primes = AVAILABLE_NTT_PRIMES;
    if (final_p_target == 1337006139375617LL) {
        candidate_primes.erase(
            std::remove_if(candidate_primes.begin(), candidate_primes.end(),
                           [final_p_target](const PrimeInfo& p) {
                               return p.mod >= final_p_target;
                           }),
            candidate_primes.end());
    }

    std::vector<PrimeInfo> sorted_available_primes = candidate_primes;
    std::sort(sorted_available_primes.begin(), sorted_available_primes.end(),
              [](const PrimeInfo& a, const PrimeInfo& b) {
        return a.mod > b.mod;
    });

    int linear_conv_len = (n_orig_coeffs == 0) ? 0 : (2 * n_orig_coeffs - 1);
    if (n_orig_coeffs == 1) linear_conv_len = 1;
    int n_fft = 1;
    if (linear_conv_len > 0) {
        while(n_fft < linear_conv_len) n_fft <<= 1;
    } else {
        if (n_orig_coeffs > 0) n_fft = 1;
    }

    int128 min_prod_needed;
    int128 p_eff_128 = final_p_target;
    int128 p_squared;

    if (p_eff_128 <= 0) {
        p_squared = 0;
    } else if (p_eff_128 > MAX_INT128_VAL / p_eff_128) { 
        p_squared = MAX_INT128_VAL;
    } else {
        p_squared = p_eff_128 * p_eff_128;
    }

    if (n_fft <= 0) {
        min_prod_needed = 1;
    } else if (p_squared == MAX_INT128_VAL) {
        min_prod_needed = MAX_INT128_VAL;
    } else if (p_squared <= 0) {
        min_prod_needed = n_fft;
    } else if ( (int128)n_fft > MAX_INT128_VAL / p_squared ) {
        min_prod_needed = MAX_INT128_VAL;
    } else {
        min_prod_needed = (int128)n_fft * p_squared;
    }

    std::vector<PrimeInfo> selected_primes_final;
    int128 current_product_final = 1;

    std::vector<PrimeInfo> best_combo_selection;
    int128 best_combo_product = 0;

    for (size_t num_to_select = 3; num_to_select <= std::min((size_t)5, sorted_available_primes.size()); ++num_to_select) {
        if (sorted_available_primes.size() < num_to_select) break;

        std::vector<size_t> p_indices(num_to_select);
        for(size_t i=0; i<num_to_select; ++i) p_indices[i]=i;
        
        bool first_iter_for_this_k = true;
        do {
            if (!first_iter_for_this_k) { 
                ptrdiff_t i = static_cast<ptrdiff_t>(num_to_select) - 1;
                while (i >= 0 && p_indices[static_cast<size_t>(i)] == sorted_available_primes.size() - num_to_select + static_cast<size_t>(i)) {
                    i--;
                }
                if (i < 0) break; 
                p_indices[static_cast<size_t>(i)]++;
                for (size_t j = static_cast<size_t>(i) + 1; j < num_to_select; j++) {
                    p_indices[j] = p_indices[j-1] + 1;
                }
            }
            first_iter_for_this_k = false;

            std::vector<PrimeInfo> current_combo_primes;
            int128 temp_product = 1;
            bool combo_overflow = false;
            for (size_t i = 0; i < num_to_select; ++i) {
                current_combo_primes.push_back(sorted_available_primes[p_indices[i]]);
                if (temp_product > 0 && sorted_available_primes[p_indices[i]].mod > 0 && temp_product > MAX_INT128_VAL / sorted_available_primes[p_indices[i]].mod) {
                    combo_overflow = true;
                    break;
                }
                temp_product *= sorted_available_primes[p_indices[i]].mod;
            }

            if (!combo_overflow && temp_product >= min_prod_needed && temp_product > best_combo_product) {
                best_combo_product = temp_product;
                best_combo_selection = current_combo_primes;
            }
        } while (true);

        if (!best_combo_selection.empty()) {
            break; 
        }
    }

    if (!best_combo_selection.empty()) {
        selected_primes_final = best_combo_selection;
        current_product_final = best_combo_product;
    } else {
        current_product_final = 1;
        for (const auto& prime : sorted_available_primes) {
            if (selected_primes_final.size() >= 8) break;
            if (current_product_final > 0 && prime.mod > 0 && current_product_final > MAX_INT128_VAL / prime.mod) {
                break; 
            }
            selected_primes_final.push_back(prime);
            current_product_final *= prime.mod;
            if (min_prod_needed != MAX_INT128_VAL && current_product_final >= min_prod_needed && current_product_final > 0) {
                 if(selected_primes_final.size() >=3) break;
            }
        }
        if (selected_primes_final.size() < 3 && current_product_final < min_prod_needed && current_product_final > 0) {
            size_t current_greedy_idx = 0;
            while(selected_primes_final.size() < 3 && current_greedy_idx < sorted_available_primes.size()) {
                bool already_selected = false;
                for(const auto& sel_p : selected_primes_final) if(sel_p.mod == sorted_available_primes[current_greedy_idx].mod) already_selected = true;
                
                if(!already_selected) {
                    if (current_product_final > 0 && sorted_available_primes[current_greedy_idx].mod > 0 && current_product_final > MAX_INT128_VAL / sorted_available_primes[current_greedy_idx].mod) {
                         break; 
                    }
                    selected_primes_final.push_back(sorted_available_primes[current_greedy_idx]);
                    current_product_final *= sorted_available_primes[current_greedy_idx].mod;
                }
                current_greedy_idx++;
            }
        }
    }
    
    if (selected_primes_final.empty() && !sorted_available_primes.empty()) {
         if (!( (current_product_final > 0 && sorted_available_primes.back().mod > 0 && current_product_final > MAX_INT128_VAL / sorted_available_primes.back().mod) || sorted_available_primes.back().mod <=0 )) {
            selected_primes_final.push_back(sorted_available_primes.back());
            current_product_final = sorted_available_primes.back().mod;
         }
    }

    if (selected_primes_final.empty()) {
        throw std::runtime_error("Could not select any CRT moduli.");
    }

    std::sort(selected_primes_final.begin(), selected_primes_final.end(), [](const PrimeInfo&a, const PrimeInfo&b){return a.mod < b.mod;});

    return selected_primes_final;
} 