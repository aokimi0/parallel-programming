#!/bin/bash

# 编译并测试脚本
# 使用方法: ./compile_and_test.sh <源文件> <线程数>
# 根据文件名自动判断测试类型

set -e

# 检查参数
if [ $# -ne 2 ]; then
    echo "使用方法: $0 <源文件> <线程数>"
    echo "  源文件: 要编译的C++源文件 (如 main.cc)"
    echo "  线程数: 并行线程数量"
    echo ""
    echo "根据文件名自动判断测试类型:"
    echo "  包含 'pthread' -> 执行 pthread 测试"
    echo "  包含 'openmp' -> 执行 openmp 测试"
    echo "  其他情况 -> 执行两种测试"
    echo ""
    echo "示例:"
    echo "  $0 main.cc 4"
    echo "  $0 ntt_pthread.cc 8"
    echo "  $0 ntt_openmp.cc 4"
    exit 1
fi

SOURCE_FILE="$1"
NUM_THREADS="$2"

# 根据文件名自动判断测试类型
if [[ "$SOURCE_FILE" == *"pthread"* ]]; then
    TEST_TYPE="pthread"
elif [[ "$SOURCE_FILE" == *"openmp"* ]]; then
    TEST_TYPE="openmp"
else
    TEST_TYPE="both"
fi

# 检查源文件是否存在
if [ ! -f "$SOURCE_FILE" ]; then
    echo "❌ 错误: 源文件 '$SOURCE_FILE' 不存在"
    exit 1
fi

# 检查线程数是否为正整数
if ! [[ "$NUM_THREADS" =~ ^[1-9][0-9]*$ ]]; then
    echo "❌ 错误: 线程数必须为正整数"
    exit 1
fi

# 检查test.sh是否存在
if [ ! -f "test.sh" ]; then
    echo "❌ 错误: test.sh 脚本不存在"
    exit 1
fi

echo "==============================================="
echo "🔧 编译并测试 NTT 实现"
echo "==============================================="
echo "📁 源文件: $SOURCE_FILE"
echo "🧵 线程数: $NUM_THREADS"
echo "🔍 自动判断测试类型: $TEST_TYPE"
echo ""

# 编译命令
COMPILE_CMD="g++ $SOURCE_FILE -o main -O2 -fopenmp -lpthread -std=c++11"
echo "⚙️  编译命令: $COMPILE_CMD"

# 执行编译
if $COMPILE_CMD; then
    echo "✅ 编译成功"
    
    # 检查可执行文件大小
    if [ -f "main" ]; then
        FILE_SIZE=$(du -h main | cut -f1)
        echo "📦 可执行文件大小: $FILE_SIZE"
    fi
    echo ""
    
    # 执行测试
    echo "🚀 开始执行测试..."
    echo ""
    
    case "$TEST_TYPE" in
        "pthread")
            echo "🔄 执行 Pthread 测试..."
            echo "命令: ./test.sh 2 1 $NUM_THREADS"
            ./test.sh 2 1 "$NUM_THREADS"
            ;;
        "openmp")
            echo "🔄 执行 OpenMP 测试..."
            echo "命令: ./test.sh 3 1 $NUM_THREADS"
            ./test.sh 3 1 "$NUM_THREADS"
            ;;
        "both")
            echo "🔄 执行 Pthread 测试..."
            echo "命令: ./test.sh 2 1 $NUM_THREADS"
            ./test.sh 2 1 "$NUM_THREADS"
            echo ""
            echo "🔄 执行 OpenMP 测试..."
            echo "命令: ./test.sh 3 1 $NUM_THREADS"
            ./test.sh 3 1 "$NUM_THREADS"
            ;;
        *)
            echo "❌ 错误: 无效的测试类型 '$TEST_TYPE'"
            echo "支持的类型: pthread, openmp, both"
            exit 1
            ;;
    esac
    
    echo ""
    echo "🎉 测试完成!"
    
else
    echo "❌ 编译失败"
    exit 1
fi 