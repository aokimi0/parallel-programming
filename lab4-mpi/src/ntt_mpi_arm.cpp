#include "common_mpi_ntt.h"
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sys/time.h>
#include <cstdint>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

using u64 = uint64_t;

void ntt_multiply_wrapper_mpi(u64 *a, u64 *b, u64 *ab, int n, u64 p, int rank, int size) {
    std::vector<ll> poly1(n), poly2(n);
    for(int i = 0; i < n; i++){
        poly1[i] = a[i];
        poly2[i] = b[i];
    }
    
    ll primitive_root = 3; 
    std::vector<ll> result = multiply_ntt_mpi(poly1, poly2, p, primitive_root, rank, size);
    
    for(int i = 0; i < n * 2 - 1; i++){
        ab[i] = result[i];
    }
}

void fRead(u64 *a, u64 *b, int *n, u64 *p, int input_id){
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strin = str1 + str2 + ".in";
    char data_path[strin.size() + 1];
    std::copy(strin.begin(), strin.end(), data_path);
    data_path[strin.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    if (!fin.is_open()) {
        if (input_id == 0) {
            *n = 4;
            *p = 7340033;
            a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
            b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;
        } else if (input_id == 1) {
            *n = 8;
            *p = 104857601;
            for(int i = 0; i < 8; i++) {
                a[i] = i + 1;
                b[i] = (i + 1) * 2;
            }
        } else if (input_id == 2) {
            *n = 16;
            *p = 469762049;
            for(int i = 0; i < 16; i++) {
                a[i] = i * i + 1;
                b[i] = (i + 2) * 3;
            }
        } else {
            *n = 32;
            *p = 7340033;
            for(int i = 0; i < 32; i++) {
                a[i] = (i * 7 + 3) % 1000;
                b[i] = (i * 5 + 1) % 1000;
            }
        }
        return;
    }
    fin >> *n >> *p;
    for (int i = 0; i < *n; i++){
        fin >> a[i];
    }
    for (int i = 0; i < *n; i++){   
        fin >> b[i];
    }
    fin.close();
}

void fCheck(u64 *ab, int n, int input_id, int rank){
    if (rank != 0) return;
    
    std::string str1 = "/nttdata/";
    std::string str2 = std::to_string(input_id);
    std::string strout = str1 + str2 + ".out";
    char data_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), data_path);
    data_path[strout.size()] = '\0';
    std::ifstream fin;
    fin.open(data_path, std::ios::in);
    if (!fin.is_open()) {
        std::cout << "测试数据文件不存在，跳过结果验证" << std::endl;
        return;
    }
    
    bool correct = true;
    for (int i = 0; i < n * 2 - 1; i++){
        u64 x;
        fin >> x;
        if(x != ab[i]){
            std::cout << "多项式乘法结果错误，位置: " << i << ", 期望: " << x << ", 实际: " << ab[i] << std::endl;
            correct = false;
            break;
        }
    }
    fin.close();
    
    if (correct) {
        std::cout << "多项式乘法结果正确" << std::endl;
    }
}

void fWrite(u64 *ab, int n, int input_id, int rank, int size){
    if (rank != 0) return;
    
    std::string str1 = "files/";
    std::string str2 = std::to_string(input_id);
    std::string str3 = "_mpi_arm_p";
    std::string str4 = std::to_string(size);
    std::string strout = str1 + str2 + str3 + str4 + ".out";
    char output_path[strout.size() + 1];
    std::copy(strout.begin(), strout.end(), output_path);
    output_path[strout.size()] = '\0';
    std::ofstream fout;
    fout.open(output_path, std::ios::out);
    for (int i = 0; i < n * 2 - 1; i++){
        fout << ab[i] << '\n';
    }
    fout.close();
}

u64 a[300000], b[300000], ab[300000];

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        std::cout << "=== NTT MPI ARM 版本测试 ===" << std::endl;
        std::cout << "MPI 进程数: " << size << std::endl;
    }
    
    int test_begin = 0;
    int test_end = 4;
    
    for(int i = test_begin; i <= test_end; ++i) {
        int n_;
        u64 p_;
        
        if (rank == 0) {
            fRead(a, b, &n_, &p_, i);
        }
        
        MPI_Bcast(&n_, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&p_, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(a, n_, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(b, n_, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
        
        memset(ab, 0, sizeof(ab));
        
        MPI_Barrier(MPI_COMM_WORLD);
        auto Start = std::chrono::high_resolution_clock::now();
        
        ntt_multiply_wrapper_mpi(a, b, ab, n_, p_, rank, size);
        
        MPI_Barrier(MPI_COMM_WORLD);
        auto End = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::ratio<1, 1000>> elapsed = End - Start;
        double ans = elapsed.count();
        
        if (rank == 0) {
            fCheck(ab, n_, i, rank);
            std::cout << "average latency for n = " << n_ << " p = " << p_ 
                     << " : " << ans << " (ms) " << std::endl;
            fWrite(ab, n_, i, rank, size);
        }
    }
    
    MPI_Finalize();
    return 0;
} 