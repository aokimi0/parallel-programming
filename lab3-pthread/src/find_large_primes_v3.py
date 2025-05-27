#!/usr/bin/env python3
"""
寻找三个大素数用于CRT，要求：
1. 形如 a×4^k+1 (用于NTT)
2. 原根为3
3. 三个素数的乘积 > np^2 = 131072 * (1337006139375617)^2 ≈ 2.34e35
"""

def pow_mod(base, exp, mod):
    """快速幂取模"""
    result = 1
    base = base % mod
    while exp > 0:
        if exp & 1:
            result = (result * base) % mod
        exp >>= 1
        base = (base * base) % mod
    return result

def is_primitive_root_3(p):
    """检查3是否为素数p的原根"""
    if pow_mod(3, p-1, p) != 1:
        return False
    
    phi = p - 1
    
    # 检查一些主要的因子
    # 对于形如a×4^k+1，phi = a×4^k，主要因子是2和a的因子
    factors_to_check = [2]
    
    # 找a的因子
    temp = phi
    while temp % 4 == 0:
        temp //= 4
    
    # 检查temp(即a)的小因子
    for i in range(3, min(100, int(temp**0.5) + 1), 2):
        if temp % i == 0:
            factors_to_check.append(i)
            while temp % i == 0:
                temp //= i
    if temp > 1:
        factors_to_check.append(temp)
    
    # 验证3^(phi/q) != 1 for all prime factors q
    for q in factors_to_check:
        if phi % q == 0:
            if pow_mod(3, phi // q, p) == 1:
                return False
    
    return True

def verify_ntt_form(p):
    """验证p是否为a×4^k+1的形式"""
    if p <= 1:
        return False, 0, 0
    
    temp = p - 1
    k = 0
    
    while temp % 4 == 0:
        temp //= 4
        k += 1
    
    if k > 0:
        a = temp
        if a * (4 ** k) + 1 == p:
            return True, a, k
    
    return False, 0, 0

def generate_potential_primes():
    """生成一些形如a×4^k+1的大数"""
    candidates = []
    
    # 手动计算一些大的候选素数
    # k从20到30，选择一些小的a值
    for k in range(20, 31):
        power_4_k = 4 ** k
        for a in [1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47]:
            p = a * power_4_k + 1
            candidates.append((p, a, k))
    
    return candidates

def miller_rabin_test(n, k=5):
    """简化的Miller-Rabin素数测试"""
    if n < 2:
        return False
    if n == 2 or n == 3:
        return True
    if n % 2 == 0:
        return False
    
    # 写成 n-1 = d * 2^r
    r = 0
    d = n - 1
    while d % 2 == 0:
        r += 1
        d //= 2
    
    # 测试几个小的底数
    for a in [2, 3, 5, 7, 11]:
        if a >= n:
            continue
        x = pow_mod(a, d, n)
        if x == 1 or x == n - 1:
            continue
        
        for _ in range(r - 1):
            x = pow_mod(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    
    return True

def main():
    print("生成并验证大NTT素数")
    print("目标：三个素数乘积 > 2.34e35")
    
    # 一些已知的NTT素数（手动验证过的）
    known_primes = [
        (998244353, 119, 23),     # 119×4^23+1
        (1004535809, 59, 24),     # 59×4^24+1  
        (2281701377, 34, 26),     # 这个需要重新计算
        (3489660929, 13, 28),     # 这个需要重新计算
    ]
    
    # 生成更多候选素数
    candidates = generate_potential_primes()
    
    print(f"开始验证候选素数...")
    valid_primes = []
    
    # 首先验证已知素数
    for p, a, k in known_primes:
        is_ntt_form, calc_a, calc_k = verify_ntt_form(p)
        if is_ntt_form and calc_a == a and calc_k == k:
            if is_primitive_root_3(p):
                valid_primes.append((p, a, k))
                print(f"已知有效素数: {p} = {a}×4^{k}+1 ({p.bit_length()}位)")
            else:
                print(f"已知素数 {p} 的原根不是3")
    
    # 验证生成的候选素数
    print(f"验证生成的候选素数...")
    count = 0
    for p, a, k in candidates:
        if count >= 20:  # 限制检查数量
            break
        
        if p.bit_length() > 60:  # 跳过太大的数
            continue
            
        if miller_rabin_test(p):
            if is_primitive_root_3(p):
                valid_primes.append((p, a, k))
                print(f"新发现有效素数: {p} = {a}×4^{k}+1 ({p.bit_length()}位)")
                count += 1
                
                if len(valid_primes) >= 5:  # 找到足够多就停止
                    break
    
    if len(valid_primes) < 3:
        print(f"只找到{len(valid_primes)}个有效素数，不足3个")
        print("尝试使用更大的范围或其他方法...")
        
        # 使用一些非常大的已知NTT素数
        very_large_primes = [
            1004535809,      # 小一点但确定有效
            998244353,       # 小一点但确定有效  
            985661441,       # 需要验证
            2013265921,      # 需要验证
            2280030209,      # 需要验证
        ]
        
        print("验证更多已知素数...")
        for p in very_large_primes:
            is_ntt_form, a, k = verify_ntt_form(p)
            if is_ntt_form and is_primitive_root_3(p):
                valid_primes.append((p, a, k))
                print(f"额外有效素数: {p} = {a}×4^{k}+1 ({p.bit_length()}位)")
        
        return
    
    # 选择三个最大的素数
    valid_primes.sort(reverse=True)
    
    # 尝试不同的组合找到满足条件的
    print(f"\n找到{len(valid_primes)}个有效素数，尝试不同组合...")
    
    best_product = 0
    best_combination = None
    
    # 检查所有可能的三元组合
    for i in range(len(valid_primes)):
        for j in range(i+1, len(valid_primes)):
            for k_idx in range(j+1, len(valid_primes)):
                p1, p2, p3 = valid_primes[i][0], valid_primes[j][0], valid_primes[k_idx][0]
                product = p1 * p2 * p3
                if product > best_product:
                    best_product = product
                    best_combination = (i, j, k_idx)
    
    if best_combination:
        selected_primes = [valid_primes[i] for i in best_combination]
        
        print(f"\n选择的三个素数:")
        product = 1
        for i, (p, a, k_val) in enumerate(selected_primes):
            print(f"M[{i}] = {p} = {a}×4^{k_val}+1 ({p.bit_length()}位)")
            product *= p
        
        print(f"\n乘积: {product}")
        print(f"乘积科学计数法: {product:.2e}")
        print(f"目标最小值: 2.34e35")
        print(f"满足要求: {product > 2.34e35}")
        
        if product > 2.34e35:
            print(f"\n✓ 找到合适的三个素数!")
        else:
            print(f"\n⚠ 乘积仍然不够大，可能需要更大的素数")
        
        print(f"\nC++代码:")
        primes_only = [p for p, a, k_val in selected_primes]
        print(f"const u64 M[3] = {{{primes_only[0]}ULL, {primes_only[1]}ULL, {primes_only[2]}ULL}};")

if __name__ == "__main__":
    main() 