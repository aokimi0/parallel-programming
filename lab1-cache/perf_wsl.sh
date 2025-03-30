#!/bin/bash

# 获取perf的路径
PERF_PATH=$(which perf 2>/dev/null)

if [ -z "$PERF_PATH" ]; then
    # 如果找不到perf，尝试使用Ubuntu 6.8内核的perf
    PERF_PATH="/usr/lib/linux-tools-6.8.0-56/perf"
    if [ ! -f "$PERF_PATH" ]; then
        echo "找不到perf工具，请安装linux-tools-common和linux-tools-generic"
        exit 1
    fi
fi

# 运行perf并传递所有参数
$PERF_PATH "$@" 