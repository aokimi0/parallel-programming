#!/bin/bash

set -e

if [ $# -lt 1 ]; then
    echo "用法: $0 源文件名 [测试次数]"
    exit 1
fi

SRC="$1"
EXE="${SRC%.*}"
TIMES="${2:-1}"

# 判断是否为 neon 版本
if [[ "${SRC,,}" == *neon* ]]; then
    COMPILER="aarch64-linux-gnu-g++"
    CFLAGS="-O2 -std=c++17 -march=armv8-a+simd -I/usr/lib/gcc-cross/aarch64-linux-gnu/13/include"
    RUNNER="qemu-aarch64 -L /usr/aarch64-linux-gnu"
else
    COMPILER="g++"
    CFLAGS="-O2 -mavx2 -std=c++17"
    RUNNER=""
fi

for MOD in 469762049 1337006139375617 7696582450348003; do
    for SIZE in 1000 10000 100000; do
        echo "=============================="
        echo "测试规模: n = m = $SIZE, mod = $MOD"
        # 1. 生成 input.txt
        python3 - <<EOF > src/input.txt
import random
n = $SIZE
mod = $MOD
print(f"{n} {mod}")
print(" ".join(str(random.randint(0, 1000)) for _ in range(n+1)))
print(" ".join(str(random.randint(0, 1000)) for _ in range(n+1)))
EOF

        # 2. 编译
        $COMPILER $CFLAGS "$SRC" -o "$EXE"

        # 3. 多次运行并测量时间（纳秒精度）
        total_ns=0
        for ((t=1; t<=TIMES; ++t)); do
            start_time=$(date +%s%N)
            $RUNNER "$EXE" < src/input.txt > src/output.txt
            end_time=$(date +%s%N)
            elapsed_ns=$((end_time - start_time))
            total_ns=$((total_ns + elapsed_ns))
        done

        avg_sec=$(awk "BEGIN {printf \"%.6f\", $total_ns/$TIMES/1000000000}")
        echo "$EXE 平均运行时间: $avg_sec 秒"

        # 4. 用 ntt.cpp 生成 answer.txt（每个规模都重新生成）
        g++ -O2 -mavx2 -std=c++17 src/ntt.cpp -o ntt_std
        ./ntt_std < src/input.txt > src/answer.txt

        # 5. 校验 output.txt 是否与 answer.txt 一致
        if diff <(tr -d ' \n' <src/output.txt) <(tr -d ' \n' <src/answer.txt) > /dev/null; then
            echo "✅ 正确！输出与 answer.txt 一致。"
        else
            echo "❌ 错误！输出与 answer.txt 不一致。"
        fi
    done
done