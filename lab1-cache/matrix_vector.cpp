#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>

const int MAXN = 4000;

// 测试数据
int matrix[MAXN][MAXN];
int vector_data[MAXN];
int result_normal[MAXN];
int result_cache[MAXN];

// 生成测试数据
void generate_data(int n) {
    for (int i = 0; i < n; i++) {
        vector_data[i] = i;
        for (int j = 0; j < n; j++) {
            matrix[i][j] = i + j;
        }
    }
}

// 重置结果数组
void reset_results(int n) {
    for (int i = 0; i < n; i++) {
        result_normal[i] = 0;
        result_cache[i] = 0;
    }
}

// 平凡算法：逐列访问矩阵元素
void column_access(int n) {
    for (int i = 0; i < n; i++) {
        result_normal[i] = 0;
        for (int j = 0; j < n; j++) {
            result_normal[i] += matrix[j][i] * vector_data[j];
        }
    }
}

// cache优化算法：逐行访问矩阵元素
void row_access(int n) {
    for (int i = 0; i < n; i++) {
        result_cache[i] = 0;
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result_cache[j] += matrix[i][j] * vector_data[i];
        }
    }
}

// 验证两种算法结果是否一致
bool verify_results(int n) {
    for (int i = 0; i < n; i++) {
        if (result_normal[i] != result_cache[i]) {
            return false;
        }
    }
    return true;
}

// 测量执行时间（纳秒）
int64_t measure_time(void (*func)(int), int n, int repeat) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < repeat; i++) {
        func(n);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / repeat;
}

int main(int argc, char* argv[]) {
    // 默认测试配置
    int algorithm = 0; // 0: 两种算法都测试, 1: 只测试平凡算法, 2: 只测试cache优化算法
    int max_size = 3000;
    int step = 500;
    
    // 处理命令行参数
    if (argc > 1) {
        algorithm = std::stoi(argv[1]);
    }
    if (argc > 2) {
        max_size = std::stoi(argv[2]);
    }
    if (argc > 3) {
        step = std::stoi(argv[3]);
    }
    
    // 设置输出格式
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "矩阵大小,平凡算法(ms),cache优化算法(ms),加速比" << std::endl;

    // 测试不同规模
    for (int n = step; n <= max_size; n += step) {
        generate_data(n);
        reset_results(n);
        
        // 根据参数决定测试哪种算法
        double time_normal = 0, time_cache = 0;
        int repeat = 3; // 重复次数，可根据规模调整
        
        if (algorithm == 0 || algorithm == 1) {
            time_normal = measure_time(column_access, n, repeat) / 1e6; // 转换为毫秒
        }
        
        if (algorithm == 0 || algorithm == 2) {
            time_cache = measure_time(row_access, n, repeat) / 1e6; // 转换为毫秒
        }
        
        double speedup = (time_normal > 0 && time_cache > 0) ? time_normal / time_cache : 0;
        
        std::cout << n << "," << time_normal << "," << time_cache << "," << speedup << std::endl;
        
        // 验证结果
        if (algorithm == 0 && !verify_results(n)) {
            std::cerr << "错误: 两种算法的结果不一致！" << std::endl;
            break;
        }
    }
    
    return 0;
} 