#include "common_mpi_ntt.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    int n_coeffs;
    ll mod_val;
    
    if (rank == 0) {
        if (!(std::cin >> n_coeffs >> mod_val)) {
            std::cerr << "Error: Failed to read input" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        if (n_coeffs <= 0) {
            std::cerr << "Error: N_COEFFS must be positive" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    
    MPI_Bcast(&n_coeffs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mod_val, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    std::vector<ll> p1(n_coeffs);
    std::vector<ll> p2(n_coeffs);
    
    if (rank == 0) {
        for (int i = 0; i < n_coeffs; ++i) {
            if (!(std::cin >> p1[i])) {
                std::cerr << "Error: Failed to read polynomial A" << std::endl;
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        for (int i = 0; i < n_coeffs; ++i) {
            if (!(std::cin >> p2[i])) {
                std::cerr << "Error: Failed to read polynomial B" << std::endl;
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }
    
    MPI_Bcast(&p1[0], n_coeffs, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&p2[0], n_coeffs, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    ll primitive_root = 3;
    
    const int num_runs = 10;
    std::vector<double> times;
    std::vector<ll> result;
    
    try {
        for(int run = 0; run < num_runs; run++) {
            MPI_Barrier(MPI_COMM_WORLD);
            double start_time = MPI_Wtime();
            
            result = multiply_ntt_mpi(p1, p2, mod_val, primitive_root, rank, size);
            
            MPI_Barrier(MPI_COMM_WORLD);
            double end_time = MPI_Wtime();
            
            double elapsed_time = (end_time - start_time) * 1000000.0;
            times.push_back(elapsed_time);
        }
        
        double total_time = 0.0;
        for(double t : times) total_time += t;
        double avg_time = total_time / num_runs;
        
        if (rank == 0) {
            for (size_t i = 0; i < result.size(); ++i) {
                std::cout << result[i] << (i == result.size() - 1 ? "" : " ");
            }
            std::cout << std::endl;
            
            std::cerr << "MPI processes: " << size << ", Average time (10 runs): " << avg_time << " microseconds" << std::endl;
        }
    } catch (const std::exception& e) {
        if (rank == 0) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    MPI_Finalize();
    return 0;
} 