#include <bits/stdc++.h>
using namespace std;

uint64_t MOD;

int main() {
    int n;
    cin >> n >> MOD;
    vector<uint64_t> a(n + 1), b(n + 1);
    for (int i = 0; i <= n; ++i) cin >> a[i];
    for (int i = 0; i <= n; ++i) cin >> b[i];
    vector<uint64_t> c(2 * n + 1);
    for (int i = 0; i <= n; ++i) {
        for (int j = 0; j <= n; ++j) {
            c[i + j] = (c[i + j] + a[i] * b[j]) % MOD;
        }
    }
    for (size_t i = 0; i < c.size(); ++i) cout << c[i] << " \n"[i + 1 == c.size()];
    return 0;
} 