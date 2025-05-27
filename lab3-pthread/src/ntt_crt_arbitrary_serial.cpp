#include "common_crt_ntt.h"
#include <vector>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random> 
#include <cstring> 

std::vector<ll> multiply_ntt_crt_linear_serial(
    const std::vector<ll>& poly_a_orig,
    const std::vector<ll>& poly_b_orig,
    int n_orig_coeffs, 
    ll final_p_target) {

    std::vector<PrimeInfo> selected_primes = select_crt_moduli(n_orig_coeffs, final_p_target);
    if (selected_primes.empty()) {
        if (n_orig_coeffs == 0 && poly_a_orig.empty() && poly_b_orig.empty()) return {};
        throw std::runtime_error("Serial: No CRT primes selected.");
    }

    int linear_conv_len = (n_orig_coeffs == 0) ? 0 : (2 * n_orig_coeffs - 1);
    if (n_orig_coeffs == 1) linear_conv_len = 1;

    size_t n_fft = 1;
    if (linear_conv_len > 0) {
        while (n_fft < static_cast<size_t>(linear_conv_len)) n_fft <<= 1;
    } else {
        if (n_orig_coeffs == 0 && poly_a_orig.empty() && poly_b_orig.empty()) n_fft = 0;
        else n_fft = 1; 
    }

    if (n_fft == 0 && linear_conv_len == 0 && n_orig_coeffs == 0 && poly_a_orig.empty() && poly_b_orig.empty()) return {};

    std::vector<ll> poly_a_padded(n_fft, 0);
    std::vector<ll> poly_b_padded(n_fft, 0);
    for (size_t i = 0; i < n_fft; ++i) {
        if (i < poly_a_orig.size()) poly_a_padded[i] = poly_a_orig[i];
        if (i < poly_b_orig.size()) poly_b_padded[i] = poly_b_orig[i];
    }

    size_t num_selected_primes = selected_primes.size();
    std::vector<std::vector<ll>> ntt_branch_results(num_selected_primes, std::vector<ll>(n_fft));

    for (size_t k = 0; k < num_selected_primes; ++k) {
        PrimeInfo current_prime_info = selected_primes[k];
        std::vector<ll> temp_a = poly_a_padded;
        std::vector<ll> temp_b = poly_b_padded;

        for (size_t i = 0; i < n_fft; ++i) {
            temp_a[i] %= current_prime_info.mod;
            if (temp_a[i] < 0) temp_a[i] += current_prime_info.mod;
            temp_b[i] %= current_prime_info.mod;
            if (temp_b[i] < 0) temp_b[i] += current_prime_info.mod;
        }

        ntt(temp_a, false, current_prime_info.mod, current_prime_info.root);
        ntt(temp_b, false, current_prime_info.mod, current_prime_info.root);

        for (size_t i = 0; i < n_fft; ++i) {
            ntt_branch_results[k][i] = (int128)temp_a[i] * temp_b[i] % current_prime_info.mod;
        }
        ntt(ntt_branch_results[k], true, current_prime_info.mod, current_prime_info.root);
    }

    std::vector<ll> final_coeffs(n_fft);
    if (n_fft > 0) {
        for (size_t i = 0; i < n_fft; ++i) { 
            if (selected_primes.empty()) {
                final_coeffs[i] = 0;
                continue;
            }
            int128 current_ans = ntt_branch_results[0][i];
            int128 current_prod_mod = selected_primes[0].mod;

            for (size_t j = 1; j < num_selected_primes; ++j) {
                ll Mj = selected_primes[j].mod;
                ll Xj = ntt_branch_results[j][i];
                
                ll term_inv_input = (ll)(current_prod_mod % Mj);
                term_inv_input = (term_inv_input % Mj + Mj) % Mj; 
                ll inv_m_prod_mod_Mj = inv_exgcd(term_inv_input, Mj);
                
                int128 diff = Xj - (current_ans % Mj);
                diff = (diff % Mj + Mj) % Mj;
                
                int128 k_crt = (diff * inv_m_prod_mod_Mj) % Mj;
                current_ans = current_ans + k_crt * current_prod_mod;

                if (current_prod_mod > 0 && Mj > 0 && current_prod_mod > MAX_INT128_VAL / Mj) {
                }
                current_prod_mod *= Mj; 
                current_ans = (current_ans % current_prod_mod + current_prod_mod) % current_prod_mod;
            }

            if (final_p_target != 0) {
                final_coeffs[i] = (ll)((current_ans % final_p_target + final_p_target) % final_p_target);
            } else { 
                 final_coeffs[i] = (ll)current_ans; 
            }
        }
    }

    final_coeffs.resize(linear_conv_len > 0 ? linear_conv_len : ((poly_a_orig.empty() || poly_b_orig.empty()) ? 0:1) );
    if (poly_a_orig.empty() || poly_b_orig.empty()) final_coeffs.clear();

    return final_coeffs;
}

std::vector<ll> multiply_naive(const std::vector<ll>& poly1, const std::vector<ll>& poly2, ll mod) {
    if (poly1.empty() || poly2.empty()) {
        return {};
    }
    int n1 = poly1.size();
    int n2 = poly2.size();
    int res_len = n1 + n2 - 1;
    if (res_len <= 0 && (n1 > 0 || n2 > 0) ) res_len = 1; 
    else if (res_len <= 0) return {};

    std::vector<ll> result(res_len, 0);
    for (int i = 0; i < n1; ++i) {
        for (int j = 0; j < n2; ++j) {
            if (mod != 0) {
                result[i + j] = (result[i + j] + (int128)poly1[i] * poly2[j]) % mod;
                if (result[i+j] < 0) result[i+j] += mod;
            } else { 
                 result[i + j] = result[i + j] + (int128)poly1[i] * poly2[j];
            }
        }
    }
    return result;
}

void print_poly(const std::string& name, const std::vector<ll>& p, std::ostream& os = std::cout) {
    os << name << " (size " << p.size() << "): [";
    for (size_t i = 0; i < p.size(); ++i) {
        os << p[i] << (i == p.size() - 1 ? "" : ", ");
        if (i > 10 && i < p.size() -1) { 
            os << "..., ";
            i = p.size() - 2;
        }
    }
    os << "]" << std::endl;
}

bool compare_polys(const std::vector<ll>& p1, const std::vector<ll>& p2) {
    if (p1.size() != p2.size()) {
        std::cerr << "Compare failed: Size mismatch " << p1.size() << " vs " << p2.size() << std::endl;
        return false;
    }
    for (size_t i = 0; i < p1.size(); ++i) {
        if (p1[i] != p2[i]) {
            std::cerr << "Compare failed: Mismatch at index " << i << ": " << p1[i] << " vs " << p2[i] << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int n_coeffs;
    ll p_target;
    std::cin >> n_coeffs >> p_target;

    std::vector<ll> poly_A(n_coeffs);
    std::vector<ll> poly_B(n_coeffs);

    for (int i = 0; i < n_coeffs; ++i) {
        std::cin >> poly_A[i];
    }
    for (int i = 0; i < n_coeffs; ++i) {
        std::cin >> poly_B[i];
    }

    try {
        std::vector<ll> result = multiply_ntt_crt_linear_serial(poly_A, poly_B, n_coeffs, p_target);
        for (size_t i = 0; i < result.size(); ++i) {
            std::cout << result[i] << (i == result.size() - 1 ? "" : " ");
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in serial execution: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 