import random

def generate_input_file(filename, n_coeffs, mod):
    with open(filename, 'w') as f:
        f.write(f"{n_coeffs} {mod}\\n")
        
        p1 = [random.randint(0, mod - 1) for _ in range(n_coeffs)]
        p2 = [random.randint(0, mod - 1) for _ in range(n_coeffs)]
        
        f.write(" ".join(map(str, p1)) + "\\n")
        f.write(" ".join(map(str, p2)) + "\\n")

if __name__ == "__main__":
    N_COEFFS = 1 << 12 # 4096
    MOD = 998244353
    FILENAME = "input.txt"
    generate_input_file(FILENAME, N_COEFFS, MOD)
    print(f"Generated input file '{FILENAME}' with n_coeffs={N_COEFFS}.") 