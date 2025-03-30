#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>

// 朴素算法：简单循环
double naive_sum(const std::vector<double>& array) {
    double sum = 0.0;
    for (size_t i = 0; i < array.size(); ++i) {
        sum += array[i];
    }
    return sum;
}

// 双链路算法：使用两个独立的累加器以增加指令级并行度
double dual_path_sum(const std::vector<double>& array) {
    double sum1 = 0.0;
    double sum2 = 0.0;
    size_t n = array.size();
    
    // 使用两个累加器
    for (size_t i = 0; i < n; i += 2) {
        sum1 += array[i];
        if (i + 1 < n) { // 防止越界
            sum2 += array[i + 1];
        }
    }
    
    return sum1 + sum2;
}

// 递归算法：分治策略，增加时间局部性
double recursive_sum_helper(const std::vector<double>& array, size_t start, size_t end) {
    if (end - start <= 1) {
        return (start < end) ? array[start] : 0.0;
    }
    
    size_t mid = start + (end - start) / 2;
    return recursive_sum_helper(array, start, mid) + 
           recursive_sum_helper(array, mid, end);
}

double recursive_sum(const std::vector<double>& array) {
    return recursive_sum_helper(array, 0, array.size());
}

int main(int argc, char* argv[]) {
    // 默认数组大小 2^24 约1677万
    size_t n = 1 << 24;
    
    // 从命令行参数获取数组大小
    if (argc > 1) {
        n = std::stoull(argv[1]);
    }
    
    std::cout << "数组大小: " << n << " 元素" << std::endl;
    
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    
    // 生成随机数组
    std::vector<double> array(n);
    for (size_t i = 0; i < n; ++i) {
        array[i] = dis(gen);
    }
    
    // 测量朴素算法性能
    auto start_time = std::chrono::high_resolution_clock::now();
    double naive_result = naive_sum(array);
    auto end_time = std::chrono::high_resolution_clock::now();
    double naive_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    std::cout << "Naive sum time: " << std::fixed << std::setprecision(2) << naive_time << " ms" << std::endl;
    
    // 测量双链路算法性能
    start_time = std::chrono::high_resolution_clock::now();
    double dual_result = dual_path_sum(array);
    end_time = std::chrono::high_resolution_clock::now();
    double dual_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    std::cout << "Dual path sum time: " << std::fixed << std::setprecision(2) << dual_time << " ms" << std::endl;
    
    // 测量递归算法性能
    start_time = std::chrono::high_resolution_clock::now();
    double recursive_result = recursive_sum(array);
    end_time = std::chrono::high_resolution_clock::now();
    double recursive_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    std::cout << "Recursive sum time: " << std::fixed << std::setprecision(2) << recursive_time << " ms" << std::endl;
    
    // 计算并输出加速比
    double dual_speedup = (dual_time > 0) ? naive_time / dual_time : 0;
    double recursive_speedup = (recursive_time > 0) ? naive_time / recursive_time : 0;
    
    std::cout << "Dual path speedup: " << std::fixed << std::setprecision(2) << dual_speedup << "x" << std::endl;
    std::cout << "Recursive speedup: " << std::fixed << std::setprecision(2) << recursive_speedup << "x" << std::endl;
    
    // 验证结果
    bool results_match = (std::abs(naive_result - dual_result) < 1e-10) && 
                         (std::abs(naive_result - recursive_result) < 1e-10);
    
    std::cout << "Results match: " << (results_match ? "Yes" : "No") << std::endl;
    
    return 0;
} 