#include <iostream>
#include <vector>
#include <numeric>

using coeff_type = long long;
using product_type = __int128;

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int n_coeffs;
    coeff_type mod;

    std::cin >> n_coeffs >> mod;

    if (n_coeffs == 0) {
        std::cout << std::endl;
        return 0;
    }
    if (n_coeffs < 0) {
         std::cerr << "Error: N_COEFFS must be non-negative." << std::endl;
         return 1;
    }

    std::vector<coeff_type> a(n_coeffs);
    std::vector<coeff_type> b(n_coeffs);

    for (int i = 0; i < n_coeffs; ++i) {
        std::cin >> a[i];
    }
    for (int i = 0; i < n_coeffs; ++i) {
        std::cin >> b[i];
    }
    
    if (mod <= 0) {
        std::cerr << "Error: Modulus must be positive." << std::endl;
        return 1;
    }

    int result_len = 2 * n_coeffs - 1;
    if (n_coeffs == 1) result_len = 1;
    if (result_len < 0) result_len = 0;
    
    std::vector<coeff_type> result(result_len, 0);

    if (n_coeffs > 0) {
        for (int i = 0; i < n_coeffs; ++i) {
            for (int j = 0; j < n_coeffs; ++j) {
                product_type term = (product_type)a[i] * b[j];
                result[i + j] = (result[i + j] + term) % mod;
            }
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] < 0) {
            result[i] += mod;
        }
        std::cout << result[i] << (i == result.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;

    return 0;
} 