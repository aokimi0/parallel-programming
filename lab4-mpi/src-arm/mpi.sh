#!/bin/bash

echo "正在编译代码..."
mpic++ -o main ./ntt_mpi_arm.cpp ./common_mpi_ntt.cpp

if [ $? -eq 0 ]; then
    echo "编译成功！"
else
    echo "编译失败，请检查错误！"
    exit 1 # 编译失败，退出脚本
fi

echo "正在提交作业 qsub_mpi.sh..."
rm -f test.o test.e

JOB_ID=$(qsub qsub_mpi.sh)

if [ -z "$JOB_ID" ]; then
    echo "作业提交失败，请检查 qsub 命令！"
    exit 1
else
    echo "作业已提交，ID 为：$JOB_ID"
    JOB_NUMBER=$(echo $JOB_ID | cut -d'.' -f1)
    echo "正在等待作业 $JOB_NUMBER 完成..."
    while true; do
        JOB_STATUS=$(qstat -f $JOB_NUMBER 2>/dev/null | grep job_state | awk '{print $3}')
        
        if [ "$JOB_STATUS" == "C" ]; then 
            echo "作业 $JOB_NUMBER 已完成。"
            break
        elif [ -z "$JOB_STATUS" ]; then 
            echo "作业 $JOB_NUMBER 已从队列中移除，假设已完成。"
            break
        elif [ "$JOB_STATUS" == "E" ] || [ "$JOB_STATUS" == "F" ]; then
            echo "作业 $JOB_NUMBER 异常终止或失败！"
            break
        fi
        sleep 10
    done
fi

# 3. 作业完成后，查看 test.o 和 test.e
echo "-----------------------------------"
echo "正在查看 test.o 的内容："
cat test.o

echo "-----------------------------------"
echo "正在查看 test.e 的内容："
cat test.e
echo "-----------------------------------"

echo "脚本执行完毕。"