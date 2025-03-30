#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>

// 普通矩阵-向量乘法（列优先访问）
void naive_matrix_vector_mult(const std::vector<std::vector<double>>& matrix, 
                             const std::vector<double>& vector,
                             std::vector<double>& result) {
    int n = matrix.size();
    for (int j = 0; j < n; j++) {  // 列优先访问
        for (int i = 0; i < n; i++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

// 缓存优化的矩阵-向量乘法（行优先访问）
void cache_optimized_matrix_vector_mult(const std::vector<std::vector<double>>& matrix, 
                                       const std::vector<double>& vector,
                                       std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {  // 行优先访问
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

// 列优先访问 (朴素算法)
void col_access(const std::vector<std::vector<double>>& matrix, 
               const std::vector<double>& vector,
               std::vector<double>& result) {
    int n = matrix.size();
    for (int j = 0; j < n; j++) {  // 列优先访问
        for (int i = 0; i < n; i++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

// 行优先访问
void row_access(const std::vector<std::vector<double>>& matrix, 
               const std::vector<double>& vector,
               std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {  // 行优先访问
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

// 循环展开5次
void unroll5(const std::vector<std::vector<double>>& matrix, 
            const std::vector<double>& vector,
            std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        int j = 0;
        // 每次迭代处理5个元素
        for (; j <= n - 5; j += 5) {
            sum += matrix[i][j] * vector[j] +
                   matrix[i][j+1] * vector[j+1] +
                   matrix[i][j+2] * vector[j+2] +
                   matrix[i][j+3] * vector[j+3] +
                   matrix[i][j+4] * vector[j+4];
        }
        // 处理剩余元素
        for (; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

// 循环展开10次
void unroll10(const std::vector<std::vector<double>>& matrix, 
             const std::vector<double>& vector,
             std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        int j = 0;
        // 每次迭代处理10个元素
        for (; j <= n - 10; j += 10) {
            sum += matrix[i][j] * vector[j] +
                   matrix[i][j+1] * vector[j+1] +
                   matrix[i][j+2] * vector[j+2] +
                   matrix[i][j+3] * vector[j+3] +
                   matrix[i][j+4] * vector[j+4] +
                   matrix[i][j+5] * vector[j+5] +
                   matrix[i][j+6] * vector[j+6] +
                   matrix[i][j+7] * vector[j+7] +
                   matrix[i][j+8] * vector[j+8] +
                   matrix[i][j+9] * vector[j+9];
        }
        // 处理剩余元素
        for (; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

// 循环展开15次
void unroll15(const std::vector<std::vector<double>>& matrix, 
             const std::vector<double>& vector,
             std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        int j = 0;
        // 每次迭代处理15个元素
        for (; j <= n - 15; j += 15) {
            sum += matrix[i][j] * vector[j] +
                   matrix[i][j+1] * vector[j+1] +
                   matrix[i][j+2] * vector[j+2] +
                   matrix[i][j+3] * vector[j+3] +
                   matrix[i][j+4] * vector[j+4] +
                   matrix[i][j+5] * vector[j+5] +
                   matrix[i][j+6] * vector[j+6] +
                   matrix[i][j+7] * vector[j+7] +
                   matrix[i][j+8] * vector[j+8] +
                   matrix[i][j+9] * vector[j+9] +
                   matrix[i][j+10] * vector[j+10] +
                   matrix[i][j+11] * vector[j+11] +
                   matrix[i][j+12] * vector[j+12] +
                   matrix[i][j+13] * vector[j+13] +
                   matrix[i][j+14] * vector[j+14];
        }
        // 处理剩余元素
        for (; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

// 循环展开20次
void unroll20(const std::vector<std::vector<double>>& matrix, 
             const std::vector<double>& vector,
             std::vector<double>& result) {
    int n = matrix.size();
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        int j = 0;
        // 每次迭代处理20个元素
        for (; j <= n - 20; j += 20) {
            sum += matrix[i][j] * vector[j] +
                   matrix[i][j+1] * vector[j+1] +
                   matrix[i][j+2] * vector[j+2] +
                   matrix[i][j+3] * vector[j+3] +
                   matrix[i][j+4] * vector[j+4] +
                   matrix[i][j+5] * vector[j+5] +
                   matrix[i][j+6] * vector[j+6] +
                   matrix[i][j+7] * vector[j+7] +
                   matrix[i][j+8] * vector[j+8] +
                   matrix[i][j+9] * vector[j+9] +
                   matrix[i][j+10] * vector[j+10] +
                   matrix[i][j+11] * vector[j+11] +
                   matrix[i][j+12] * vector[j+12] +
                   matrix[i][j+13] * vector[j+13] +
                   matrix[i][j+14] * vector[j+14] +
                   matrix[i][j+15] * vector[j+15] +
                   matrix[i][j+16] * vector[j+16] +
                   matrix[i][j+17] * vector[j+17] +
                   matrix[i][j+18] * vector[j+18] +
                   matrix[i][j+19] * vector[j+19];
        }
        // 处理剩余元素
        for (; j < n; j++) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

int main(int argc, char* argv[]) {
    // 默认矩阵大小
    int n = 1000;
    // 运行模式：0=全部, 1=col, 2=row, 3=unroll5, 4=unroll10, 5=unroll15, 6=unroll20
    int mode = 0;
    
    // 解析命令行参数
    if (argc > 1) {
        n = std::stoi(argv[1]);
    }
    if (argc > 2) {
        mode = std::stoi(argv[2]);
    }
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // 创建并初始化矩阵
    std::vector<std::vector<double>> matrix(n, std::vector<double>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = dis(gen);
        }
    }
    
    // 创建并初始化向量
    std::vector<double> vector(n);
    for (int i = 0; i < n; i++) {
        vector[i] = dis(gen);
    }
    
    // 结果向量
    std::vector<double> result(n, 0.0);
    std::vector<double> result_verify(n, 0.0); // 用于验证结果

    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // 运行所选算法并计时
    if (mode == 0 || mode == 1) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        col_access(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto col_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Col access time: " << std::fixed << std::setprecision(2) << col_time << " ms" << std::endl;
        result_verify = result; // 保存结果用于验证
    }
    
    if (mode == 0 || mode == 2) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        row_access(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto row_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Row access time: " << std::fixed << std::setprecision(2) << row_time << " ms" << std::endl;
        
        // 验证结果
        if (mode == 0) {
            bool match = true;
            for (int i = 0; i < n && match; i++) {
                if (std::abs(result[i] - result_verify[i]) > 1e-10) {
                    match = false;
                }
            }
            std::cout << "Row result matches: " << (match ? "Yes" : "No") << std::endl;
        }
    }
    
    if (mode == 0 || mode == 3) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        unroll5(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto unroll5_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Unroll5 time: " << std::fixed << std::setprecision(2) << unroll5_time << " ms" << std::endl;
        
        // 验证结果
        if (mode == 0) {
            bool match = true;
            for (int i = 0; i < n && match; i++) {
                if (std::abs(result[i] - result_verify[i]) > 1e-10) {
                    match = false;
                }
            }
            std::cout << "Unroll5 result matches: " << (match ? "Yes" : "No") << std::endl;
        }
    }
    
    if (mode == 0 || mode == 4) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        unroll10(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto unroll10_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Unroll10 time: " << std::fixed << std::setprecision(2) << unroll10_time << " ms" << std::endl;
        
        // 验证结果
        if (mode == 0) {
            bool match = true;
            for (int i = 0; i < n && match; i++) {
                if (std::abs(result[i] - result_verify[i]) > 1e-10) {
                    match = false;
                }
            }
            std::cout << "Unroll10 result matches: " << (match ? "Yes" : "No") << std::endl;
        }
    }
    
    if (mode == 0 || mode == 5) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        unroll15(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto unroll15_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Unroll15 time: " << std::fixed << std::setprecision(2) << unroll15_time << " ms" << std::endl;
        
        // 验证结果
        if (mode == 0) {
            bool match = true;
            for (int i = 0; i < n && match; i++) {
                if (std::abs(result[i] - result_verify[i]) > 1e-10) {
                    match = false;
                }
            }
            std::cout << "Unroll15 result matches: " << (match ? "Yes" : "No") << std::endl;
        }
    }
    
    if (mode == 0 || mode == 6) {
        result.assign(n, 0.0);
        start_time = std::chrono::high_resolution_clock::now();
        unroll20(matrix, vector, result);
        end_time = std::chrono::high_resolution_clock::now();
        auto unroll20_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        std::cout << "Unroll20 time: " << std::fixed << std::setprecision(2) << unroll20_time << " ms" << std::endl;
        
        // 验证结果
        if (mode == 0) {
            bool match = true;
            for (int i = 0; i < n && match; i++) {
                if (std::abs(result[i] - result_verify[i]) > 1e-10) {
                    match = false;
                }
            }
            std::cout << "Unroll20 result matches: " << (match ? "Yes" : "No") << std::endl;
        }
    }
    
    return 0;
} 