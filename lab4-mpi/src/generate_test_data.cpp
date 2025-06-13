#include <iostream>
#include <vector>
#include <random>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <n_coeffs> <mod> <output_file>" << std::endl;
        return 1;
    }
    
    int n_coeffs = std::stoi(argv[1]);
    long long mod = std::stoll(argv[2]);
    std::string filename = argv[3];
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<long long> dis(1, mod - 1);
    
    std::ofstream file(filename);
    file << n_coeffs << " " << mod << std::endl;
    
    for (int i = 0; i < n_coeffs; i++) {
        file << dis(gen) << (i == n_coeffs - 1 ? "" : " ");
    }
    file << std::endl;
    
    for (int i = 0; i < n_coeffs; i++) {
        file << dis(gen) << (i == n_coeffs - 1 ? "" : " ");
    }
    file << std::endl;
    
    file.close();
    return 0;
} 