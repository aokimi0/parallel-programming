#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <cmath>

const int MAXN = 16777216; // 2^24

// 测试数据
int data[MAXN];
int result;

// 生成测试数据
void generate_data(int n) {
    for (int i = 0; i < n; i++) {
        data[i] = i;
    }
}

// 平凡算法：逐个累加
void single_link(int n) {
    int res = 0;
    for (int i = 0; i < n; i++) {
        res += data[i];
    }
    result = res;
}

// 双链路算法：两路并行累加
void double_link(int n) {
    int res1 = 0, res2 = 0;
    for (int i = 0; i < n; i += 2) {
        res1 += data[i];
        res2 += data[i + 1];
    }
    result = res1 + res2;
}

// 递归算法：两两相加
void recursive_sum(int n) {
    // 确保n是2的幂
    int m = 1;
    while (m < n) m *= 2;
    
    // 复制数据到临时数组
    int* temp = new int[m];
    for (int i = 0; i < n; i++) {
        temp[i] = data[i];
    }
    for (int i = n; i < m; i++) {
        temp[i] = 0; // 补零
    }
    
    // 两两相加
    for (int step = m; step > 1; step /= 2) {
        for (int i = 0; i < step/2; i++) {
            temp[i] = temp[i*2] + temp[i*2 + 1];
        }
    }
    
    result = temp[0];
    delete[] temp;
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
    int algorithm = 0; // 0: 三种算法都测试, 1: 平凡算法, 2: 双链路算法, 3: 递归算法
    int max_power = 24; // 最大规模 2^24
    
    // 处理命令行参数
    if (argc > 1) {
        algorithm = std::stoi(argv[1]);
    }
    if (argc > 2) {
        max_power = std::stoi(argv[2]);
    }
    
    // 设置输出格式
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "数组大小,平凡算法(ms),双链路算法(ms),递归算法(ms),双链路加速比,递归加速比" << std::endl;

    // 测试不同规模 (2的幂)
    for (int p = 16; p <= max_power; p += 2) {
        int n = 1 << p; // 2^p
        generate_data(n);
        
        // 根据参数决定测试哪种算法
        double time_single = 0, time_double = 0, time_recursive = 0;
        int repeat = std::max(1, 5000000 / n); // 根据规模调整重复次数
        
        if (algorithm == 0 || algorithm == 1) {
            time_single = measure_time(single_link, n, repeat) / 1e6; // 转换为毫秒
        }
        
        if (algorithm == 0 || algorithm == 2) {
            time_double = measure_time(double_link, n, repeat) / 1e6; // 转换为毫秒
        }
        
        if (algorithm == 0 || algorithm == 3) {
            time_recursive = measure_time(recursive_sum, n, repeat) / 1e6; // 转换为毫秒
        }
        
        double speedup_double = (time_single > 0 && time_double > 0) ? time_single / time_double : 0;
        double speedup_recursive = (time_single > 0 && time_recursive > 0) ? time_single / time_recursive : 0;
        
        std::cout << n << "," << time_single << "," << time_double << "," 
                  << time_recursive << "," << speedup_double << "," 
                  << speedup_recursive << std::endl;
    }
    
    return 0;
} 