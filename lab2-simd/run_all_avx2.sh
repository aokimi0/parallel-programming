#!/bin/bash

OUT_CSV="src/avx2_benchmark.csv"
echo "filename,mod,size,avg_sec,correct" > "$OUT_CSV"

# 查找所有avx2相关和基线ntt.cpp、dif.cpp文件
for SRC in src/ntt.cpp src/dif.cpp $(ls src/*avx2*.cpp 2>/dev/null); do
    if [ ! -f "$SRC" ]; then
        continue
    fi
    # 调用 test.sh 测试，输出日志
    bash src/test.sh "$SRC" 1 | tee tmp_test.log

    # 解析输出并写入csv
    awk -v file="$SRC" '
        /测试规模:/ {
            for(i=1;i<=NF;i++) {
                if($i=="n") size=$(i+2);
                if($i=="mod") mod=$(i+2);
            }
        }
        /平均运行时间:/ {split($0,a,": "); avg=a[2]; correct="N/A"}
        /一致/ {correct="yes"}
        /不一致/ {correct="no"}
        /平均运行时间:/ {print file "," mod "," size "," avg "," correct}
    ' tmp_test.log >> "$OUT_CSV"
done

rm -f tmp_test.log
echo "结果已保存到 $OUT_CSV"