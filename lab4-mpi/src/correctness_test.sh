#!/bin/bash

echo "=================== 正确性测试 ==================="

make clean && make all

if [ ! -f ntt_mpi ] || [ ! -f ntt_serial_reference ]; then
    echo "编译失败，退出测试"
    exit 1
fi

MODS=(7340033 104857601 469762049)
SIZES=(4 8 16 32 64 128)
PROCESSES=(1 2 4)

TEST_COUNT=0
PASS_COUNT=0

mkdir -p correctness_results

echo "开始正确性对拍测试..."
echo "测试结果摘要：" > correctness_results/summary.txt

for mod in "${MODS[@]}"; do
    for size in "${SIZES[@]}"; do
        echo "测试：size=$size, mod=$mod"
        
        ./generate_test_data $size $mod "test_${size}_${mod}.txt"
        
        echo "运行串行参考程序..."
        timeout 30s ./ntt_serial_reference < "test_${size}_${mod}.txt" > "correctness_results/serial_${size}_${mod}.out" 2>/dev/null
        
        if [ $? -ne 0 ]; then
            echo "串行程序运行失败，跳过此测试"
            continue
        fi
        
        for proc in "${PROCESSES[@]}"; do
            TEST_COUNT=$((TEST_COUNT + 1))
            echo "运行MPI程序 (进程数=$proc)..."
            
            timeout 30s mpiexec -n $proc ./ntt_mpi < "test_${size}_${mod}.txt" 2>/dev/null | head -1 > "correctness_results/mpi_${proc}_${size}_${mod}.out"
            
            if [ $? -ne 0 ]; then
                echo "MPI程序运行失败"
                echo "FAIL: size=$size, mod=$mod, proc=$proc - MPI程序运行失败" >> correctness_results/summary.txt
                continue
            fi
            
            if diff -q "correctness_results/serial_${size}_${mod}.out" "correctness_results/mpi_${proc}_${size}_${mod}.out" > /dev/null; then
                echo "✓ PASS: size=$size, mod=$mod, proc=$proc"
                echo "PASS: size=$size, mod=$mod, proc=$proc" >> correctness_results/summary.txt
                PASS_COUNT=$((PASS_COUNT + 1))
            else
                echo "✗ FAIL: size=$size, mod=$mod, proc=$proc"
                echo "FAIL: size=$size, mod=$mod, proc=$proc - 输出不匹配" >> correctness_results/summary.txt
                echo "串行输出：" >> correctness_results/summary.txt
                cat "correctness_results/serial_${size}_${mod}.out" >> correctness_results/summary.txt
                echo "MPI输出：" >> correctness_results/summary.txt
                cat "correctness_results/mpi_${proc}_${size}_${mod}.out" >> correctness_results/summary.txt
                echo "---" >> correctness_results/summary.txt
            fi
        done
        
        rm -f "test_${size}_${mod}.txt"
    done
done

echo ""
echo "=================== 测试完成 ==================="
echo "总测试数: $TEST_COUNT"
echo "通过测试: $PASS_COUNT"
echo "失败测试: $((TEST_COUNT - PASS_COUNT))"
echo "通过率: $(echo "scale=2; $PASS_COUNT * 100 / $TEST_COUNT" | bc)%"

echo "" >> correctness_results/summary.txt
echo "总测试数: $TEST_COUNT" >> correctness_results/summary.txt
echo "通过测试: $PASS_COUNT" >> correctness_results/summary.txt
echo "失败测试: $((TEST_COUNT - PASS_COUNT))" >> correctness_results/summary.txt
echo "通过率: $(echo "scale=2; $PASS_COUNT * 100 / $TEST_COUNT" | bc)%" >> correctness_results/summary.txt

if [ $PASS_COUNT -eq $TEST_COUNT ]; then
    echo "✓ 所有测试通过！可以进行性能测试。"
    echo "✓ 所有测试通过！" >> correctness_results/summary.txt
    exit 0
else
    echo "✗ 存在失败测试，请检查实现。"
    echo "✗ 存在失败测试" >> correctness_results/summary.txt
    exit 1
fi 