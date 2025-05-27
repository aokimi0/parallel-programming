#!/bin/bash

# 综合测试脚本 - 使用指定参数测试NTT实现
# 测试参数: n=1000,10000,100000，模数= 7340033 104857601 469762049 1337006139375617

# set -x # 如果存在，则注释掉
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

TIMES="${1:-3}"
RESULTS_DIR="fig"
FIG_DIR="fig"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
SUMMARY_FILE="$RESULTS_DIR/综合测试报告_$TIMESTAMP.txt"
CSV_FILE="$RESULTS_DIR/性能数据_$TIMESTAMP.csv"

mkdir -p "$RESULTS_DIR"

SIZES=(1000 10000 100000)
MODULI=(7340033 104857601 469762049 1337006139375617 100000000000000000 1000000000000000000 5000000000000000000)
THREAD_COUNTS=(1 2 4 8)

PLAIN_NTT_SOURCES=(
    "src/ntt_serial.cpp"
    "src/ntt_pthread.cpp"
    "src/ntt_pthread_optimized.cpp"
    "src/ntt_openmp.cpp"
)

CRT_ARBITRARY_NTT_SOURCES=(
    "src/ntt_crt_arbitrary_serial.cpp"
    "src/ntt_crt_arbitrary_pthread.cpp"
    "src/ntt_crt_arbitrary_openmp.cpp"
)


ALL_SOURCE_FILES=(
    "src/common_crt_ntt.cpp"
    "src/naive_poly_mult.cpp" 
    "${PLAIN_NTT_SOURCES[@]}"
    "${CRT_ARBITRARY_NTT_SOURCES[@]}"
)


REFERENCE_IMPL_PLAIN="src/ntt_serial" 
REFERENCE_IMPL_CRT_OLD="src/ntt_crt_arbitrary_serial" # Keep old name for potential backward compatibility or specific tests
REFERENCE_IMPL_NAIVE_POLY_MULT="src/naive_poly_mult" # For CRT against very large moduli

# echo "==========================================="
# echo "🚀 开始综合性能测试"
# echo "测试规模 (N_coeffs): ${SIZES[*]}"
# echo "测试模数/目标模数: ${MODULI[*]}"
# echo "运行次数: $TIMES"
# echo "参考实现 (普通NTT): $REFERENCE_IMPL_PLAIN"
# echo "参考实现 (CRT任意模 - 传统): $REFERENCE_IMPL_CRT_OLD"
# echo "参考实现 (CRT任意模 - 朴素验证): $REFERENCE_IMPL_NAIVE_POLY_MULT"
# echo "时间戳: $TIMESTAMP"
# echo "==========================================="
echo "🚀 开始综合性能测试 (时间戳: $TIMESTAMP)"

# 初始化汇总报告
cat > "$SUMMARY_FILE" <<EOF
NTT实现综合性能测试报告
生成时间: $(date)
测试运行次数: $TIMES 次
参考实现 (普通NTT): $REFERENCE_IMPL_PLAIN
参考实现 (CRT任意模 - 传统): $REFERENCE_IMPL_CRT_OLD
参考实现 (CRT任意模 - 朴素验证): $REFERENCE_IMPL_NAIVE_POLY_MULT

====================================
测试环境信息
====================================
操作系统: $(uname -a)
编译器: $(g++ --version | head -n1)
CPU信息: $(grep "model name" /proc/cpuinfo | head -n1 | cut -d':' -f2 | xargs)
内核数: $(nproc)

====================================
测试参数配置
====================================
测试规模: ${SIZES[*]}
测试模数: ${MODULI[*]}
线程配置: ${THREAD_COUNTS[*]}

====================================
详细测试结果
====================================

EOF

echo "实现,规模,模数,线程数,平均时间(秒),标准差,最小时间,最大时间,结果验证" > "$CSV_FILE"

compare_results() {
    local file1="$1"
    local file2="$2"
    
    if [ ! -f "$file1" ] || [ ! -f "$file2" ]; then
        return 1
    fi
    
    if diff -w "$file1" "$file2" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# echo "🔧 编译所有实现..."

COMMON_CRT_NTT_OBJ="src/common_crt_ntt.o"
# echo "编译 src/common_crt_ntt.cpp..."
if g++ -std=c++11 -O2 -g -Wall -Wextra -c src/common_crt_ntt.cpp -o "$COMMON_CRT_NTT_OBJ"; then
    echo "✅ common_crt_ntt.o 编译成功"
else
    echo "❌ common_crt_ntt.o 编译失败. 测试终止."
    exit 1
fi

for SRC_FILE in "${ALL_SOURCE_FILES[@]}"; do
    if [ "$SRC_FILE" == "src/common_crt_ntt.cpp" ]; then
        continue
    fi
    # Skip naive_poly_mult from this generic loop if it's handled separately or as a main
    if [ "$SRC_FILE" == "src/naive_poly_mult.cpp" ]; then
        EXE_BASE_NAME="${SRC_FILE%.*}"
        EXE_NAME=$(basename "$EXE_BASE_NAME")
        EXE_PATH="src/$EXE_NAME"
        # echo "编译 $SRC_FILE 到 $EXE_PATH..."
        if g++ -std=c++11 -O2 -g -Wall -Wextra "$SRC_FILE" -o "$EXE_PATH"; then
            echo "✅ $EXE_NAME 编译成功"
        else
            echo "❌ $EXE_NAME 编译失败"
        fi
        continue # Move to next file
    fi

    if [ ! -f "$SRC_FILE" ]; then
        # echo "⚠️  警告: $SRC_FILE 未找到，跳过"
        continue
    fi
    
    EXE_BASE_NAME="${SRC_FILE%.*}"
    EXE_NAME=$(basename "$EXE_BASE_NAME")
    EXE_PATH="src/$EXE_NAME" 
    
    # echo "编译 $SRC_FILE 到 $EXE_PATH..."
    
    COMPILE_FLAGS="-std=c++11 -O2 -g -Wall -Wextra" 
    MAIN_MACRO=""
    LINKER_FLAGS=""
    ADDITIONAL_OBJS=""
    
    case "$EXE_NAME" in
        ntt_serial)
            MAIN_MACRO="-DCOMPILE_MAIN_NTT_SERIAL"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_pthread)
            COMPILE_FLAGS+=" -pthread"
            LINKER_FLAGS+="-lpthread"
            MAIN_MACRO="-DCOMPILE_MAIN_NTT_PTHREAD"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_pthread_optimized)
            COMPILE_FLAGS+=" -pthread"
            LINKER_FLAGS+="-lpthread"
            MAIN_MACRO="-DCOMPILE_MAIN_NTT_PTHREAD_OPTIMIZED"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_openmp)
            COMPILE_FLAGS+=" -fopenmp"
            LINKER_FLAGS+="-fopenmp"
            MAIN_MACRO="-DCOMPILE_MAIN_NTT_OPENMP"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_crt_arbitrary_serial)
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_crt_arbitrary_pthread)
            COMPILE_FLAGS+=" -pthread"
            LINKER_FLAGS+="-lpthread"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
        ntt_crt_arbitrary_openmp)
            COMPILE_FLAGS+=" -fopenmp"
            LINKER_FLAGS+="-fopenmp"
            ADDITIONAL_OBJS="$COMMON_CRT_NTT_OBJ"
            ;;
    esac
    
    if [ -z "$MAIN_MACRO" ] && [[ "$EXE_NAME" == "ntt_crt_arbitrary_serial" || "$EXE_NAME" == "ntt_crt_arbitrary_pthread" || "$EXE_NAME" == "ntt_crt_arbitrary_openmp" ]]; then
      # echo "g++ $COMPILE_FLAGS "$SRC_FILE" $ADDITIONAL_OBJS $LINKER_FLAGS -o "$EXE_PATH" -lm"
      if g++ $COMPILE_FLAGS "$SRC_FILE" $ADDITIONAL_OBJS $LINKER_FLAGS -o "$EXE_PATH" -lm; then
          echo "✅ $EXE_NAME 编译成功"
      else
          echo "❌ $EXE_NAME 编译失败"
          continue
      fi
    elif [ ! -z "$MAIN_MACRO" ]; then
      # echo "g++ $COMPILE_FLAGS $MAIN_MACRO "$SRC_FILE" $ADDITIONAL_OBJS $LINKER_FLAGS -o "$EXE_PATH" -lm"
      if g++ $COMPILE_FLAGS $MAIN_MACRO "$SRC_FILE" $ADDITIONAL_OBJS $LINKER_FLAGS -o "$EXE_PATH" -lm; then
          echo "✅ $EXE_NAME 编译成功"
      else
          echo "❌ $EXE_NAME 编译失败"
          continue
      fi
    else
        : # Explicitly do nothing for this case, was previously only a comment
        # echo "ℹ️  $SRC_FILE not an explicit executable target in case statement and no MAIN_MACRO, assuming not to compile directly here or already handled."
    fi
done

# echo ""
# echo "🧪 开始性能测试..."

for SIZE in "${SIZES[@]}"; do
    for MOD in "${MODULI[@]}"; do
        echo ""
        echo "========================================"
        echo "测试配置: n=$SIZE, mod=$MOD"
        echo "========================================"
        
        INPUT_FILE="src/input_${SIZE}_${MOD}.txt"
        python3 - <<EOF > "$INPUT_FILE"
import random
n_coeffs_val = $SIZE 
mod_val = $MOD
print(f"{n_coeffs_val} {mod_val}") # Output N_COEFFS and MOD
max_coeff = min(100000, mod_val - 1 if mod_val > 0 else 100000)
if max_coeff <= 0: max_coeff = 1 # Ensure max_coeff is at least 1 if mod_val is 1 or less
# Generate n_coeffs_val coefficients for each polynomial
coeffs_a = [str(random.randint(0, max_coeff)) for _ in range(n_coeffs_val)]
coeffs_b = [str(random.randint(0, max_coeff)) for _ in range(n_coeffs_val)]
print(" ".join(coeffs_a))
print(" ".join(coeffs_b))
EOF

        REFERENCE_OUTPUT_DIR="src/ref_outputs"
        mkdir -p "$REFERENCE_OUTPUT_DIR"
        REFERENCE_PLAIN_OUTPUT="${REFERENCE_OUTPUT_DIR}/reference_plain_${SIZE}_${MOD}.txt"
        REFERENCE_CRT_NAIVE_OUTPUT="${REFERENCE_OUTPUT_DIR}/reference_crt_naive_${SIZE}_${MOD}.txt" # For CRT verification with naive mult
        
        # echo "🔍 生成参考结果..."

        # Generate reference for plain NTTs
        REF_PLAIN_EXE_PATH=$(basename "$REFERENCE_IMPL_PLAIN")
        if [ -f "$REFERENCE_PLAIN_OUTPUT" ]; then
            echo "  ➡️  复用已存在的参考结果 ($REF_PLAIN_EXE_PATH): $REFERENCE_PLAIN_OUTPUT"
        elif [ -f "src/$REF_PLAIN_EXE_PATH" ]; then
            # echo "  生成 $REF_PLAIN_EXE_PATH 结果 (N_coeffs=$SIZE, mod=$MOD)..."
            if ! "src/$REF_PLAIN_EXE_PATH" < "$INPUT_FILE" > "$REFERENCE_PLAIN_OUTPUT" 2>/tmp/ref_plain_err.txt; then
                echo "⚠️ $REF_PLAIN_EXE_PATH 执行失败或不适用 (错误见 /tmp/ref_plain_err.txt) for ($SIZE, $MOD). This might be expected for non-NTT moduli."
                # Do not erase the output if it failed; it might be partially useful or indicate an issue.
            else
                echo "  ✅ 参考结果 ($REF_PLAIN_EXE_PATH) 生成于 $REFERENCE_PLAIN_OUTPUT" # 保持成功提示
            fi
        else
            echo "❌ 参考实现 $REF_PLAIN_EXE_PATH 可执行文件 (src/$REF_PLAIN_EXE_PATH) 未找到。"
        fi

        # Generate reference for CRT NTTs using naive_poly_mult
        REF_NAIVE_EXE_PATH=$(basename "$REFERENCE_IMPL_NAIVE_POLY_MULT")
        if [ -f "$REFERENCE_CRT_NAIVE_OUTPUT" ]; then
            echo "  ➡️  复用已存在的参考结果 ($REF_NAIVE_EXE_PATH): $REFERENCE_CRT_NAIVE_OUTPUT"
        elif [ -f "src/$REF_NAIVE_EXE_PATH" ]; then
            # echo "  生成 $REF_NAIVE_EXE_PATH 结果 (N_coeffs=$SIZE, target_mod=$MOD)..."
            if ! "src/$REF_NAIVE_EXE_PATH" < "$INPUT_FILE" > "$REFERENCE_CRT_NAIVE_OUTPUT" 2>/tmp/ref_crt_naive_err.txt; then
                echo "❌ $REF_NAIVE_EXE_PATH 执行失败 (错误见 /tmp/ref_crt_naive_err.txt)，跳过依赖此参考的测试 ($SIZE, $MOD)"
            else
                echo "  ✅ 参考结果 ($REF_NAIVE_EXE_PATH) 生成于 $REFERENCE_CRT_NAIVE_OUTPUT" # 保持成功提示
            fi
        else
            echo "❌ 参考实现 $REF_NAIVE_EXE_PATH 可执行文件 (src/$REF_NAIVE_EXE_PATH) 未找到。"
        fi
        
        # Decision point for proceeding with tests for this SIZE, MOD combination
        # We need at least one relevant reference output to proceed with specific test types.
        # If plain output failed but naive CRT output is OK, CRT tests can still run.
        # If naive CRT output failed, CRT tests cannot run.

        for SRC_FILE_FULL_PATH in "${ALL_SOURCE_FILES[@]}"; do
            if [[ "$SRC_FILE_FULL_PATH" == *"common_crt_ntt.cpp"* ]] || [[ "$SRC_FILE_FULL_PATH" == *"naive_poly_mult.cpp"* ]]; then
                continue
            fi

            if [ ! -f "$SRC_FILE_FULL_PATH" ]; then
                continue
            fi
            
            EXE_BASE_NAME="${SRC_FILE_FULL_PATH%.*}"
            EXE_NAME=$(basename "$EXE_BASE_NAME")
            EXE_PATH="src/$EXE_NAME"
            
            echo "" # Add a newline before each test block for better readability
            echo "测试 $EXE_NAME (源: $SRC_FILE_FULL_PATH)..."

            ACTUAL_REFERENCE_OUTPUT_TO_USE=""
            IS_PLAIN_NTT_TYPE=false
            IS_CRT_ARBITRARY_NTT_TYPE=false
            REFERENCE_TYPE_DESCRIPTION="" # For logging

            # 确定当前EXE是Plain NTT还是CRT NTT类型
            for plain_src in "${PLAIN_NTT_SOURCES[@]}"; do
                if [ "$SRC_FILE_FULL_PATH" == "$plain_src" ]; then
                    IS_PLAIN_NTT_TYPE=true
                    break
                fi
            done

            if ! $IS_PLAIN_NTT_TYPE; then 
                for crt_arb_src in "${CRT_ARBITRARY_NTT_SOURCES[@]}"; do
                    if [ "$SRC_FILE_FULL_PATH" == "$crt_arb_src" ]; then
                        IS_CRT_ARBITRARY_NTT_TYPE=true
                        break
                    fi
                done
            fi

            # 设置期望的参考文件，Plain NTT 优先用 Plain Ref，若无则回退到 Naive Ref
            if [ "$IS_PLAIN_NTT_TYPE" = true ]; then
                if [ -f "$REFERENCE_PLAIN_OUTPUT" ]; then
                    ACTUAL_REFERENCE_OUTPUT_TO_USE="$REFERENCE_PLAIN_OUTPUT"
                    REFERENCE_TYPE_DESCRIPTION="Plain NTT Reference ($REFERENCE_PLAIN_OUTPUT)"
                elif [ -f "$REFERENCE_CRT_NAIVE_OUTPUT" ]; then
                    echo "    ℹ️  Plain NTT reference $REFERENCE_PLAIN_OUTPUT not found (ntt_serial may have failed for this modulus)."
                    echo "    Fallback: Using Naive CRT reference $REFERENCE_CRT_NAIVE_OUTPUT for $EXE_NAME."
                    ACTUAL_REFERENCE_OUTPUT_TO_USE="$REFERENCE_CRT_NAIVE_OUTPUT"
                    REFERENCE_TYPE_DESCRIPTION="Fallback Naive CRT Reference for Plain NTT ($REFERENCE_CRT_NAIVE_OUTPUT)"
                else
                    echo "    ⚠️  Neither Plain NTT reference ($REFERENCE_PLAIN_OUTPUT) nor Naive CRT reference ($REFERENCE_CRT_NAIVE_OUTPUT) found. Cannot determine reference for $EXE_NAME."
                    # This case will be caught by the later check for ACTUAL_REFERENCE_OUTPUT_TO_USE being empty or file not existing.
                fi
            elif [ "$IS_CRT_ARBITRARY_NTT_TYPE" = true ]; then
                if [ -f "$REFERENCE_CRT_NAIVE_OUTPUT" ]; then
                    ACTUAL_REFERENCE_OUTPUT_TO_USE="$REFERENCE_CRT_NAIVE_OUTPUT"
                    REFERENCE_TYPE_DESCRIPTION="CRT Naive Reference ($REFERENCE_CRT_NAIVE_OUTPUT)"
                else
                    echo "    ⚠️  Naive CRT reference $REFERENCE_CRT_NAIVE_OUTPUT not found. Cannot determine reference for $EXE_NAME."
                fi
            fi
            
            # DEBUGGING: Print state before skip checks
            echo "    [DEBUG SKIP CHECK] For $EXE_NAME: IS_PLAIN_NTT_TYPE=$IS_PLAIN_NTT_TYPE, IS_CRT_ARBITRARY_NTT_TYPE=$IS_CRT_ARBITRARY_NTT_TYPE"
            if [ -f "$REFERENCE_PLAIN_OUTPUT" ]; then
                echo "    [DEBUG SKIP CHECK] REFERENCE_PLAIN_OUTPUT ($REFERENCE_PLAIN_OUTPUT) exists."
            else
                echo "    [DEBUG SKIP CHECK] REFERENCE_PLAIN_OUTPUT ($REFERENCE_PLAIN_OUTPUT) DOES NOT exist."
            fi
            if [ -f "$REFERENCE_CRT_NAIVE_OUTPUT" ]; then
                echo "    [DEBUG SKIP CHECK] REFERENCE_CRT_NAIVE_OUTPUT ($REFERENCE_CRT_NAIVE_OUTPUT) exists."
            else
                echo "    [DEBUG SKIP CHECK] REFERENCE_CRT_NAIVE_OUTPUT ($REFERENCE_CRT_NAIVE_OUTPUT) DOES NOT exist."
            fi

            # 如果是 plain NTT 类型，但其参考文件未能生成，则跳过
            # 此特定跳过条件已由上面的回退逻辑覆盖，现在依赖于后续的 ACTUAL_REFERENCE_OUTPUT_TO_USE 检查
            # if [ "$IS_PLAIN_NTT_TYPE" = true ] && [ ! -f "$REFERENCE_PLAIN_OUTPUT" ]; then
            #     echo "    ⚠️  Plain NTT 参考输出 $REFERENCE_PLAIN_OUTPUT 未生成 (ntt_serial可能对此模数失败)，跳过 $EXE_NAME 的Plain NTT测试。"
            #     echo "$EXE_NAME,$SIZE,$MOD,N/A,N/A,N/A,N/A,N/A,SKIPPED_NO_PLAIN_REF" >> "$CSV_FILE"
            #     echo "$EXE_NAME: n=$SIZE, mod=$MOD, Plain NTT参考未生成，测试跳过" >> "$SUMMARY_FILE"
            #     continue 
            # fi

            # 如果是 CRT Arbitrary NTT 类型，但其参考文件未能生成，则跳过
            if [ "$IS_CRT_ARBITRARY_NTT_TYPE" = true ] && [ ! -f "$REFERENCE_CRT_NAIVE_OUTPUT" ]; then
                echo "    ⚠️  CRT Naive 参考输出 $REFERENCE_CRT_NAIVE_OUTPUT 未生成 (naive_poly_mult可能失败)，跳过 $EXE_NAME 的CRT测试。"
                echo "$EXE_NAME,$SIZE,$MOD,N/A,N/A,N/A,N/A,N/A,SKIPPED_NO_CRT_REF" >> "$CSV_FILE"
                echo "$EXE_NAME: n=$SIZE, mod=$MOD, CRT Naive参考未生成，测试跳过" >> "$SUMMARY_FILE"
                continue # 跳过此可执行文件对此 (SIZE, MOD) 的测试
            fi
            
            # 如果既不是Plain也不是CRT，或者未能确定参考，则跳过 (理论上不应发生，因为ALL_SOURCE_FILES已筛选)
            if [ -z "$ACTUAL_REFERENCE_OUTPUT_TO_USE" ] && { [ "$IS_PLAIN_NTT_TYPE" = true ] || [ "$IS_CRT_ARBITRARY_NTT_TYPE" = true ]; }; then
                 # This case should ideally not be hit if the above checks are comprehensive
                 : # do nothing, will be caught by the next check
            elif [ -z "$ACTUAL_REFERENCE_OUTPUT_TO_USE" ]; then
                echo "    ⚠️  无法确定 $EXE_NAME ($SRC_FILE_FULL_PATH) 的参考输出文件类型，跳过此实现。"
                continue 
            fi
            
            # 再次检查最终确定的参考文件是否存在 (可能在上面的逻辑中被赋值了，但文件本身可能由于某种原因不存在)
            if [ ! -f "$ACTUAL_REFERENCE_OUTPUT_TO_USE" ]; then
                echo "    ❌ 所选的参考输出文件 $ACTUAL_REFERENCE_OUTPUT_TO_USE 不存在，跳过 $EXE_NAME 的测试。"
                echo "$EXE_NAME,$SIZE,$MOD,N/A,N/A,N/A,N/A,N/A,SKIPPED_REF_MISSING" >> "$CSV_FILE"
                echo "$EXE_NAME: n=$SIZE, mod=$MOD, 所选参考 ($ACTUAL_REFERENCE_OUTPUT_TO_USE) 不存在，测试跳过" >> "$SUMMARY_FILE"
                continue
            fi
            
            # echo "    使用参考: $REFERENCE_TYPE_DESCRIPTION"

            IS_PARALLEL=0
            case "$EXE_NAME" in
                *pthread*|*openmp*)
                    IS_PARALLEL=1
                    ;;
            esac
            
            if [ $IS_PARALLEL -eq 1 ]; then
                for THREAD_COUNT in "${THREAD_COUNTS[@]}"; do
                    # echo "  测试 $THREAD_COUNT 线程..."
                    
                    TEST_OUTPUT_DIR="src/test_outputs"
                    mkdir -p "$TEST_OUTPUT_DIR"
                    TEST_OUTPUT="${TEST_OUTPUT_DIR}/test_${EXE_NAME}_${THREAD_COUNT}_${SIZE}_${MOD}.txt"
                    RESULT_STATUS="FAIL"
                    
                    # echo "    执行: $EXE_PATH $THREAD_COUNT < $INPUT_FILE > $TEST_OUTPUT"
                    # 初步验证运行，错误输出到特定文件
                    VALIDATION_ERR_FILE="/tmp/validate_err_${EXE_NAME}_${THREAD_COUNT}.txt"
                    rm -f "$VALIDATION_ERR_FILE" 
                    if "$EXE_PATH" $THREAD_COUNT < "$INPUT_FILE" > "$TEST_OUTPUT" 2>"$VALIDATION_ERR_FILE"; then
                        if compare_results "$ACTUAL_REFERENCE_OUTPUT_TO_USE" "$TEST_OUTPUT"; then
                            RESULT_STATUS="PASS"
                            # echo "    ✅ 结果验证通过"
                        else
                            echo "    ❌ 结果验证失败 (与 $ACTUAL_REFERENCE_OUTPUT_TO_USE 比较)"
                            echo "    参考结果和实际结果不匹配，跳过性能测试。错误日志: $VALIDATION_ERR_FILE (如果存在)"
                            RESULT_STATUS="FAIL_VALIDATION"
                            # echo "    参考: $ACTUAL_REFERENCE_OUTPUT_TO_USE" 
                            # echo "    实际: $TEST_OUTPUT (保留供检查)"
                            # 即使验证失败，也写入CSV，但标记为FAIL_VALIDATION，不进行计时
                            echo "$EXE_NAME,$SIZE,$MOD,$THREAD_COUNT,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE"
                            echo "$EXE_NAME (${THREAD_COUNT}线程): n=$SIZE, mod=$MOD, 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                            rm -f "$TEST_OUTPUT" # 清理测试输出
                            continue # 跳过对此线程数的计时
                        fi
                    else
                        echo "    ❌ 程序执行失败 (错误见 $VALIDATION_ERR_FILE)"
                        RESULT_STATUS="FAIL_EXECUTION"
                        # 程序执行失败，也写入CSV，标记为FAIL_EXECUTION，不进行计时
                        echo "$EXE_NAME,$SIZE,$MOD,$THREAD_COUNT,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE"
                        echo "$EXE_NAME (${THREAD_COUNT}线程): n=$SIZE, mod=$MOD, 结果验证=$RESULT_STATUS (执行失败)" >> "$SUMMARY_FILE"
                        rm -f "$TEST_OUTPUT" # 清理测试输出
                        continue # 跳过对此线程数的计时
                    fi
                    
                    rm -f "$TEST_OUTPUT" # 验证通过后，清理测试输出文件
                    rm -f "$VALIDATION_ERR_FILE" # 清理验证错误日志
                    
                    times=()
                    valid_runs=0
                    timed_run_errors=0
                    for ((run_idx=1; run_idx<=TIMES; run_idx++)); do
                        start_time_raw=""
                        end_time_raw=""
                        elapsed_sec="skip_unknown" # 默认跳过状态
                        
                        TEMP_ERR_FILE_TIMING_LOOP="/tmp/timing_run_err_${EXE_NAME}_${THREAD_COUNT}_${run_idx}.txt"
                        rm -f "$TEMP_ERR_FILE_TIMING_LOOP"

                        start_time_raw=$(date +%s%N)
                        
                        if ! "$EXE_PATH" $THREAD_COUNT < "$INPUT_FILE" > /dev/null 2>"$TEMP_ERR_FILE_TIMING_LOOP"; then
                            echo "    ⚠️ 警告: $EXE_NAME (线程 $THREAD_COUNT, 第 $run_idx 次计时运行) 执行失败 (错误见 $TEMP_ERR_FILE_TIMING_LOOP)" >&2
                            elapsed_sec="skip_exec_fail_in_timing"
                            timed_run_errors=$((timed_run_errors + 1))
                        else
                            end_time_raw=$(date +%s%N)
                            if ! [[ "$start_time_raw" =~ ^[0-9]+$ ]]; then
                                echo "    ⚠️ 警告: $EXE_NAME (线程 $THREAD_COUNT, 第 $run_idx 次计时运行) start_time 无效: '$start_time_raw'" >&2
                                elapsed_sec="skip_invalid_start_ts"
                            elif ! [[ "$end_time_raw" =~ ^[0-9]+$ ]]; then
                                echo "    ⚠️ 警告: $EXE_NAME (线程 $THREAD_COUNT, 第 $run_idx 次计时运行) end_time 无效: '$end_time_raw'" >&2
                                elapsed_sec="skip_invalid_end_ts"
                            elif [ "$start_time_raw" -gt "$end_time_raw" ]; then
                                echo "    ⚠️ 警告: $EXE_NAME (线程 $THREAD_COUNT, 第 $run_idx 次计时运行) end_time (${end_time_raw}) 小于 start_time (${start_time_raw})." >&2
                                elapsed_sec="skip_negative_diff_anomaly"
                            elif [ "$start_time_raw" -eq "$end_time_raw" ]; then
                                elapsed_sec="0.000000" # 执行时间为0
                            else
                                elapsed_sec=$(awk "BEGIN {printf \"%.6f\", ($end_time_raw - $start_time_raw) / 1000000000}")
                            fi
                        fi
                        
                        if [[ "$elapsed_sec" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then # 检查是否为有效数字 (整数或浮点数)
                            times+=("$elapsed_sec")
                            valid_runs=$((valid_runs + 1))
                        else
                             echo "    [DEBUG] 第 $run_idx 次计时运行的 elapsed_sec 无效: '$elapsed_sec'" >&2
                        fi
                        # rm -f "$TEMP_ERR_FILE_TIMING_LOOP" # 可选：如果需要保留每次计时运行的错误日志，则注释掉这行
                    done
                    
                    if [ "$valid_runs" -gt 0 ]; then
                        avg_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{sum(times_float)/len(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        min_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{min(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        max_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{max(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        std_dev=$(python3 -c "import sys, math; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; avg=sum(times_float)/len(times_float) if len(times_float) > 0 else 0; var=sum((x-avg)**2 for x in times_float)/len(times_float) if len(times_float) > 0 else 0; print(f'{math.sqrt(var):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        
                        # 更新 RESULT_STATUS 如果所有计时运行都失败了
                        if [ "$timed_run_errors" -eq "$TIMES" ] && [ "$valid_runs" -eq 0 ]; then
                           RESULT_STATUS="FAIL_TIMING_ALL_RUNS"
                        elif [ "$timed_run_errors" -gt 0 ]; then
                           RESULT_STATUS="PASS_WITH_TIMING_ISSUES (${valid_runs}/${TIMES} valid)"
                        fi

                        echo "    平均时间: ${avg_time}s (标准差: ${std_dev}s) -- ${valid_runs}/${TIMES} 次有效运行"
                        echo "$EXE_NAME,$SIZE,$MOD,$THREAD_COUNT,$avg_time,$std_dev,$min_time,$max_time,$RESULT_STATUS" >> "$CSV_FILE"
                        echo "$EXE_NAME (${THREAD_COUNT}线程): n=$SIZE, mod=$MOD, 平均时间=${avg_time}s (${valid_runs}/${TIMES} 次有效), 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                    else
                        echo "    ⚠️ 所有计时运行均无效，无法计算平均时间。"
                        RESULT_STATUS="FAIL_TIMING_NO_VALID_RUNS"
                        echo "$EXE_NAME,$SIZE,$MOD,$THREAD_COUNT,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE"
                        echo "$EXE_NAME (${THREAD_COUNT}线程): n=$SIZE, mod=$MOD, 平均时间=N/A (无有效计时), 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                    fi
                done
            else # 串行测试逻辑 (IS_PARALLEL -eq 0)
                echo "  串行测试..."
                SHOULD_SKIP_VERIFICATION="false"
                RESULT_STATUS="FAIL" # 默认失败

                REF_PLAIN_EXE_NAME_ONLY=$(basename "$REFERENCE_IMPL_PLAIN") 
                # echo "    [DEBUG] REF_PLAIN_EXE_NAME_ONLY is $REF_PLAIN_EXE_NAME_ONLY for $EXE_NAME"

                # 如果当前串行程序是 plain reference (ntt_serial)，则跳过对自身的验证
                if [ "${IS_PLAIN_NTT_TYPE}" = "true" ] && [ "${EXE_NAME}" == "${REF_PLAIN_EXE_NAME_ONLY}" ]; then
                    SHOULD_SKIP_VERIFICATION="true"
                    # echo "    ℹ️  $EXE_NAME 是 Plain NTT 参考实现，跳过自身结果验证。"
                    RESULT_STATUS="PASS_REF_SELF_SKIP" # 自身跳过验证，认为是PASS
                fi
                
                # 如果当前串行程序是 crt naive reference (naive_poly_mult), 也跳过
                REF_NAIVE_EXE_NAME_ONLY=$(basename "$REFERENCE_IMPL_NAIVE_POLY_MULT")
                if [ "${EXE_NAME}" == "${REF_NAIVE_EXE_NAME_ONLY}" ]; then # naive_poly_mult.cpp 本身就是参考
                    SHOULD_SKIP_VERIFICATION="true"
                    # echo "    ℹ️  $EXE_NAME 是 Naive Poly Mult 参考实现，跳过自身结果验证。"
                    RESULT_STATUS="PASS_REF_SELF_SKIP"
                fi

                # 如果当前串行程序是 crt arbitrary serial reference (ntt_crt_arbitrary_serial)，并且正在与 naive_poly_mult 的结果比较，那它不是"自我验证"
                # 但是，如果它是被用来作为其他CRT并行版本的参考，它也不应该"自我验证" （这个逻辑由 ACTUAL_REFERENCE_OUTPUT_TO_USE 控制）
                # 此处主要处理 ntt_serial 作为 plain ntt 的参考，以及 naive_poly_mult 作为任意模数的参考。

                if [ "${SHOULD_SKIP_VERIFICATION}" != "true" ]; then
                    TEST_OUTPUT_DIR="src/test_outputs"
                    mkdir -p "$TEST_OUTPUT_DIR"
                    TEST_OUTPUT="${TEST_OUTPUT_DIR}/test_${EXE_NAME}_${SIZE}_${MOD}.txt"
                    VALIDATION_ERR_FILE="/tmp/validate_err_${EXE_NAME}_serial.txt"
                    rm -f "$VALIDATION_ERR_FILE"
                    
                    # echo "    执行验证: $EXE_PATH < $INPUT_FILE > $TEST_OUTPUT"
                    if "$EXE_PATH" < "$INPUT_FILE" > "$TEST_OUTPUT" 2>"$VALIDATION_ERR_FILE"; then
                        if compare_results "$ACTUAL_REFERENCE_OUTPUT_TO_USE" "$TEST_OUTPUT"; then
                            RESULT_STATUS="PASS"
                            # echo "    ✅ 结果验证通过 (与 $ACTUAL_REFERENCE_OUTPUT_TO_USE 比较)"
                        else
                            RESULT_STATUS="FAIL_VALIDATION"
                            echo "    ❌ 结果验证失败 (与 $ACTUAL_REFERENCE_OUTPUT_TO_USE 比较). 错误日志: $VALIDATION_ERR_FILE (如果存在)"
                            echo "$EXE_NAME,$SIZE,$MOD,1,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE" # 串行线程数为1
                            echo "$EXE_NAME (串行): n=$SIZE, mod=$MOD, 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                            rm -f "$TEST_OUTPUT"
                            continue # 跳过计时
                        fi
                    else
                        RESULT_STATUS="FAIL_EXECUTION"
                        echo "    ❌ 程序执行失败 (错误见 $VALIDATION_ERR_FILE)"
                        echo "$EXE_NAME,$SIZE,$MOD,1,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE"
                        echo "$EXE_NAME (串行): n=$SIZE, mod=$MOD, 结果验证=$RESULT_STATUS (执行失败)" >> "$SUMMARY_FILE"
                        rm -f "$TEST_OUTPUT"
                        continue # 跳过计时
                    fi
                    rm -f "$TEST_OUTPUT" # 清理验证输出
                    rm -f "$VALIDATION_ERR_FILE" # 清理验证错误日志
                fi # end SHOULD_SKIP_VERIFICATION
                
                # 只有验证通过 (PASS 或 PASS_REF_SELF_SKIP) 才进行计时
                if [[ "$RESULT_STATUS" == "PASS" || "$RESULT_STATUS" == "PASS_REF_SELF_SKIP" ]]; then
                    times=()
                    valid_runs=0
                    timed_run_errors=0
                    for ((run_idx=1; run_idx<=TIMES; run_idx++)); do
                        start_time_raw=""
                        end_time_raw=""
                        elapsed_sec="skip_unknown"

                        TEMP_ERR_FILE_TIMING_LOOP="/tmp/timing_run_err_${EXE_NAME}_serial_${run_idx}.txt"
                        rm -f "$TEMP_ERR_FILE_TIMING_LOOP"
                        start_time_raw=$(date +%s%N)

                        if ! "$EXE_PATH" < "$INPUT_FILE" > /dev/null 2>"$TEMP_ERR_FILE_TIMING_LOOP"; then
                             echo "    ⚠️ 警告: $EXE_NAME (串行, 第 $run_idx 次计时运行) 执行失败 (错误见 $TEMP_ERR_FILE_TIMING_LOOP)" >&2
                             elapsed_sec="skip_exec_fail_in_timing"
                             timed_run_errors=$((timed_run_errors + 1))
                        else
                            end_time_raw=$(date +%s%N)
                            if ! [[ "$start_time_raw" =~ ^[0-9]+$ ]]; then
                                echo "    ⚠️ 警告: $EXE_NAME (串行, 第 $run_idx 次计时运行) start_time 无效: '$start_time_raw'" >&2
                                elapsed_sec="skip_invalid_start_ts"
                            elif ! [[ "$end_time_raw" =~ ^[0-9]+$ ]]; then
                                echo "    ⚠️ 警告: $EXE_NAME (串行, 第 $run_idx 次计时运行) end_time 无效: '$end_time_raw'" >&2
                                elapsed_sec="skip_invalid_end_ts"
                            elif [ "$start_time_raw" -gt "$end_time_raw" ]; then
                                echo "    ⚠️ 警告: $EXE_NAME (串行, 第 $run_idx 次计时运行) end_time (${end_time_raw}) 小于 start_time (${start_time_raw})." >&2
                                elapsed_sec="skip_negative_diff_anomaly"
                            elif [ "$start_time_raw" -eq "$end_time_raw" ]; then
                                elapsed_sec="0.000000"
                            else
                                elapsed_sec=$(awk "BEGIN {printf \"%.6f\", ($end_time_raw - $start_time_raw) / 1000000000}")
                            fi
                        fi
                        
                        if [[ "$elapsed_sec" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                            times+=("$elapsed_sec")
                            valid_runs=$((valid_runs + 1))
                        else
                             echo "    [DEBUG] 第 $run_idx 次串行计时运行的 elapsed_sec 无效: '$elapsed_sec'" >&2
                        fi
                        # rm -f "$TEMP_ERR_FILE_TIMING_LOOP"
                    done

                    if [ "$valid_runs" -gt 0 ]; then
                        avg_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{sum(times_float)/len(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        min_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{min(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        max_time=$(python3 -c "import sys; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; print(f'{max(times_float):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        std_dev=$(python3 -c "import sys, math; times_str=sys.argv[1:]; times_float=[float(x) for x in times_str if x.replace('.', '', 1).isdigit() and x != 'N/A']; avg=sum(times_float)/len(times_float) if len(times_float) > 0 else 0; var=sum((x-avg)**2 for x in times_float)/len(times_float) if len(times_float) > 0 else 0; print(f'{math.sqrt(var):.6f}' if len(times_float) > 0 else 'N/A')" "${times[@]}")
                        
                        if [ "$timed_run_errors" -eq "$TIMES" ] && [ "$valid_runs" -eq 0 ]; then
                           RESULT_STATUS="FAIL_TIMING_ALL_RUNS"
                        elif [ "$timed_run_errors" -gt 0 ]; then
                           RESULT_STATUS="PASS_WITH_TIMING_ISSUES (${valid_runs}/${TIMES} valid)"
                        fi

                        echo "    平均时间: ${avg_time}s (标准差: ${std_dev}s) -- ${valid_runs}/${TIMES} 次有效运行"
                        echo "$EXE_NAME,$SIZE,$MOD,1,$avg_time,$std_dev,$min_time,$max_time,$RESULT_STATUS" >> "$CSV_FILE" # 串行线程数为1
                        echo "$EXE_NAME (串行): n=$SIZE, mod=$MOD, 平均时间=${avg_time}s (${valid_runs}/${TIMES} 次有效), 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                    else
                        echo "    ⚠️ 所有计时运行均无效，无法计算平均时间 (串行)。"
                        # 保留之前的验证状态，但计时失败
                        if [[ "$RESULT_STATUS" != "FAIL_VALIDATION" && "$RESULT_STATUS" != "FAIL_EXECUTION" ]]; then
                            RESULT_STATUS="FAIL_TIMING_NO_VALID_RUNS"
                        fi
                        echo "$EXE_NAME,$SIZE,$MOD,1,N/A,N/A,N/A,N/A,$RESULT_STATUS" >> "$CSV_FILE"
                        echo "$EXE_NAME (串行): n=$SIZE, mod=$MOD, 平均时间=N/A (无有效计时), 结果验证=$RESULT_STATUS" >> "$SUMMARY_FILE"
                    fi
                fi # end if RESULT_STATUS is PASS for timing
            fi
        done
        
        rm -f "$INPUT_FILE"
        # Keep reference outputs for now, remove them if they are regenerated every time anyway
        # rm -f "$REFERENCE_PLAIN_OUTPUT" 
        # rm -f "$REFERENCE_CRT_NAIVE_OUTPUT"
    done
done

echo "" >> "$SUMMARY_FILE"
echo "=====================================" >> "$SUMMARY_FILE"
echo "测试完成时间: $(date)" >> "$SUMMARY_FILE"
echo "性能数据文件: $CSV_FILE" >> "$SUMMARY_FILE"
echo "说明: 性能数据针对的是各自参考实现验证通过的测试" >> "$SUMMARY_FILE"
echo "=====================================" >> "$SUMMARY_FILE"

echo ""
echo "🎉 测试完成!"
echo "📊 汇总报告: $SUMMARY_FILE"
echo "📈 性能数据: $CSV_FILE"
echo ""
echo "💡 注意: 性能数据基于对应类型的参考实现进行验证"
echo "下一步: 运行可视化脚本生成图表 (请确保脚本存在且配置正确)"
echo "python3 src/generate_visualizations_chinese_optimized.py \"$CSV_FILE\"" 