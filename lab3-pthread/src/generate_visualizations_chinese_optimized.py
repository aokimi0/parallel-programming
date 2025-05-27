#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NTT性能测试可视化脚本 - 中文字体优化版本
基于系统实际字体情况优化配置
"""

import os
import warnings
warnings.filterwarnings('ignore')

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import pandas as pd
import numpy as np
import glob
import seaborn as sns
import logging
import argparse
from collections import defaultdict

# 全局定义输出目录
OUTPUT_DIR = "fig"

# 禁用字体相关日志
logging.getLogger('matplotlib.font_manager').setLevel(logging.CRITICAL)

def setup_optimal_chinese_font():
    print("🔧 设置最优中文字体配置...")
    font_files = [
        '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc',
        '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc',
        '/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc'
    ]
    
    for font_path in font_files:
        if os.path.exists(font_path):
            try:
                fm.fontManager.addfont(font_path)
            except:
                pass
    
    plt.rcParams['font.sans-serif'] = [
        'WenQuanYi Zen Hei',
        'Noto Sans CJK JP',
        'Noto Serif CJK JP',
        'DejaVu Sans',
        'sans-serif'
    ]
    
    plt.rcParams['axes.unicode_minus'] = False
    plt.rcParams['figure.figsize'] = (12, 8)
    plt.rcParams['savefig.dpi'] = 300
    plt.rcParams['savefig.bbox'] = 'tight'
    
    try:
        fig, ax = plt.subplots(figsize=(1, 1))
        ax.text(0.5, 0.5, '中文测试', fontsize=1)
        plt.close(fig)
        print("✅ 中文字体配置成功")
        print(f"📝 主要字体: {plt.rcParams['font.sans-serif'][0]}")
        return True
    except:
        print("⚠️ 字体配置有问题，使用默认设置")
        return False

font_success = setup_optimal_chinese_font()

sns.set_style("whitegrid", {"font.sans-serif": plt.rcParams['font.sans-serif']})
sns.set_palette("husl")

def load_performance_data(csv_file_path):
    try:
        data = pd.read_csv(csv_file_path)
    except FileNotFoundError:
        print(f"错误：CSV文件未找到于 {csv_file_path}")
        return None
    
    # 使用CSV中的实际列名 "平均时间(秒)"
    actual_time_column_name = "平均时间(秒)"
    if actual_time_column_name not in data.columns:
        print(f"错误：CSV文件中未找到期望的列 '{actual_time_column_name}'")
        print(f"可用的列: {data.columns.tolist()}")
        # 尝试备用英文名，以防万一
        if "Time" in data.columns:
            print("尝试使用备用列名 'Time'")
            actual_time_column_name = "Time"
        else:
            return None # 如果关键列缺失，则无法继续

    data['Time'] = pd.to_numeric(data[actual_time_column_name], errors='coerce')
    data.dropna(subset=['Time'], inplace=True)

    # 同样，检查其他列名，这里假设它们是 "规模", "线程数", "实现", "模数"
    # 如果这些列名在CSV中也是中文，也需要相应调整
    actual_size_column_name = "规模"
    actual_threads_column_name = "线程数"
    actual_impl_column_name = "实现"
    actual_modulus_column_name = "模数"

    # 重命名列为代码中期望的英文名，以便后续代码统一处理
    rename_map = {
        actual_time_column_name: 'Time',
        actual_size_column_name: 'Size',
        actual_threads_column_name: 'Threads',
        actual_impl_column_name: 'Implementation',
        actual_modulus_column_name: 'Modulus'
        # 如果还有其他列，例如 "标准差", "结果验证" 等，也可以在这里添加映射
        # "标准差": "StdDev",
        # "结果验证": "ValidationStatus"
    }
    data.rename(columns=rename_map, inplace=True)

    # Ensure 'Time' column is unique after renaming, keep the first if duplicates exist
    if isinstance(data.columns, pd.MultiIndex):
        # If columns are MultiIndex, this logic might need adjustment based on its structure
        # For now, assume we are dealing with a flat column index for typical CSVs
        pass
    elif 'Time' in data.columns:
        time_cols = [col for col in data.columns if col == 'Time']
        if len(time_cols) > 1:
            print(f"警告: 在load_performance_data中发现重复的 'Time' 列 ({len(time_cols)}个)。将只保留第一个。")
            # Get all columns, then reorder to keep the first 'Time' and other unique columns
            cols = list(data.columns)
            first_time_idx = cols.index('Time')
            cols_to_keep = []
            seen_time = False
            for i, col_name in enumerate(cols):
                if col_name == 'Time':
                    if not seen_time:
                        cols_to_keep.append(col_name)
                        seen_time = True
                else:
                    if col_name not in cols_to_keep: # Keep other unique columns
                         cols_to_keep.append(col_name)
            data = data[cols_to_keep]

    # 现在可以安全地使用 'Size', 'Threads', 'Implementation', 'Modulus'
    data['Size'] = data['Size'].astype(int)
    data['Threads'] = data['Threads'].astype(int)
    
    def get_base_impl(name):
        if 'crt_arbitrary' in name:
            return name.split('_crt_arbitrary_')[-1] + '_crt'
        return name.replace('ntt_', '') # 简化名称

    data['BaseImpl'] = data['Implementation'].apply(get_base_impl)
    data['FullImpl'] = data['Implementation'] # 保存原始实现名称

    def sanitize_modulus(mod):
        if isinstance(mod, str) and mod.upper() == "CRT":
            return "CRT"
        try:
            return int(mod)
        except ValueError:
            return str(mod) # 保留无法转换的字符串形式

    data['Modulus'] = data['Modulus'].apply(sanitize_modulus)
    
    return data

def create_chinese_performance_charts(data):
    if data is None or data.empty:
        print("无数据可供绘图。")
        return {}

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    chart_paths = {}

    # Calculate average performance
    # Using as_index=False to ensure 'FullImpl' is a column, then mean should produce a DataFrame with 'FullImpl' and 'Time'
    avg_performance_df = data.groupby('FullImpl', as_index=False)['Time'].mean()

    # --- Debugging Start ---
    # print("\n--- Debugging avg_performance_df ---")
    # print(f"Columns of avg_performance_df: {avg_performance_df.columns.tolist()}")
    # print("Head of avg_performance_df:")
    # print(avg_performance_df.head())
    # print("--- Debugging End ---\n")
    # --- Debugging End ---

    # Correct avg_performance_df columns if 'Time' is duplicated
    if list(avg_performance_df.columns) == ['FullImpl', 'Time', 'Time']:
        print("ℹ️ Correcting DataFrame with duplicate 'Time' columns: selecting 'FullImpl' and the first 'Time' column.")
        avg_performance_df = avg_performance_df.iloc[:, :2] # Selects first two columns
        avg_performance_df.columns = ['FullImpl', 'Time'] # Rename them to ensure uniqueness
    elif 'Time' not in avg_performance_df.columns:
        # This case should ideally not be reached if groupby worked as expected or data loading was correct
        print(f"❌关键错误: 'Time' 列在 avg_performance_df 中不存在。检测到的列: {avg_performance_df.columns.tolist()}")
        print(f"avg_performance_df 的内容:\n{avg_performance_df.head()}")
        return {} # Cannot proceed

    # Now avg_performance_df should be a DataFrame with columns 'FullImpl' and 'Time' (mean of original Time)
    # Sort this DataFrame by the 'Time' column
    avg_performance_sorted_df = avg_performance_df.sort_values(by='Time', ascending=True)

    # For plotting with plot(kind='bar'), we need a Series where index is 'FullImpl' and values are 'Time'
    avg_performance_series = avg_performance_sorted_df.set_index('FullImpl')['Time']
        
    plt.figure(figsize=(12, 7))
    avg_performance_series.plot(kind='bar', color=['skyblue', 'lightcoral', 'lightgreen', 'gold', 'orchid', 'turquoise'])
    plt.title('各种NTT实现的平均执行时间对比', fontsize=16)
    plt.xlabel('实现方法', fontsize=14)
    plt.ylabel('平均执行时间 (秒)', fontsize=14)
    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, "avg_performance_comparison_chinese.png")
    plt.savefig(path)
    plt.close()
    chart_paths['avg_performance'] = path
    print(f"图表已保存到: {path}")

    selected_impls = [
        'ntt_serial', 'ntt_pthread', 'ntt_openmp',
        'ntt_crt_arbitrary_serial', 'ntt_crt_arbitrary_pthread', 'ntt_crt_arbitrary_openmp'
    ]
    selected_impls = [impl for impl in selected_impls if impl in data['FullImpl'].unique()]

    if selected_impls:
        max_threads_data = data.loc[data.groupby(['FullImpl', 'Size'])['Threads'].idxmax()]
        serial_data = data[data['Threads'] == 1]
        plot_data_size_impact = pd.concat([max_threads_data[max_threads_data['FullImpl'].isin(selected_impls)],
                                           serial_data[serial_data['FullImpl'].isin(selected_impls)]])
        plot_data_size_impact.drop_duplicates(subset=['FullImpl', 'Size'], keep='first', inplace=True)


        pivot_data_size = plot_data_size_impact.pivot_table(index='Size', columns='FullImpl', values='Time', aggfunc='mean')
        
        if not pivot_data_size.empty:
            pivot_data_size.plot(kind='bar', figsize=(14, 8), colormap='viridis')
            plt.title('不同问题规模 (N) 下各实现性能对比 (最大线程数/串行)', fontsize=16)
            plt.xlabel('问题规模 (N)', fontsize=14)
            plt.ylabel('平均执行时间 (秒)', fontsize=14)
            plt.xticks(rotation=45)
            plt.legend(title='实现方法', bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.tight_layout()
            path = os.path.join(OUTPUT_DIR, "size_impact_on_performance_chinese.png")
            plt.savefig(path)
            plt.close()
            chart_paths['size_impact'] = path
            print(f"图表已保存到: {path}")
        else:
            print("警告: 没有足够的数据为 'size_impact_on_performance_chinese.png' 生成透视表。")


    parallel_impls = [impl for impl in selected_impls if 'serial' not in impl]
    if parallel_impls:
        if data['Size'].nunique() > 1:
            target_size = data['Size'].quantile(0.5, interpolation='nearest') # 中位数
        else:
            target_size = data['Size'].unique()[0]
        
        plain_ntt_data = data[~data['FullImpl'].str.contains('crt')]
        if not plain_ntt_data.empty:
            unique_moduli_for_plain = plain_ntt_data['Modulus'].unique()
            numeric_moduli_series = pd.to_numeric(pd.Series(unique_moduli_for_plain), errors='coerce').dropna()

            if not numeric_moduli_series.empty:
                unique_numeric_moduli = sorted(numeric_moduli_series.unique())
                target_modulus_plain = unique_numeric_moduli[len(unique_numeric_moduli) // 2]
            elif len(unique_moduli_for_plain) > 0:
                target_modulus_plain = plain_ntt_data['Modulus'].mode()[0] if not plain_ntt_data['Modulus'].mode().empty else "Unknown"
            else:
                target_modulus_plain = "Unknown"

            if target_modulus_plain == "Unknown":
                print("警告: 在普通NTT数据中未找到可用的模数用于线程影响图。")
            else:
                data_for_thread_plot_plain = plain_ntt_data[
                    (plain_ntt_data['FullImpl'].isin(parallel_impls)) &
                    (plain_ntt_data['Size'] == target_size) &
                    (plain_ntt_data['Modulus'] == target_modulus_plain)
                ]

                if not data_for_thread_plot_plain.empty:
                    pivot_data_threads_plain = data_for_thread_plot_plain.pivot_table(index='Threads', columns='FullImpl', values='Time', aggfunc='mean')
                    if not pivot_data_threads_plain.empty:
                        pivot_data_threads_plain.plot(kind='line', marker='o', figsize=(12, 7))
                        plt.title(f'不同线程数对普通NTT性能的影响 (N={target_size}, P={target_modulus_plain})', fontsize=16)
                        plt.xlabel('线程数', fontsize=14)
                        plt.ylabel('平均执行时间 (秒)', fontsize=14)
                        plt.xticks(data_for_thread_plot_plain['Threads'].unique())
                        plt.legend(title='实现方法')
                        plt.grid(True)
                        path = os.path.join(OUTPUT_DIR, f"thread_impact_on_performance_plain_N{target_size}_P{str(target_modulus_plain).replace('.', '_')}_chinese.png")
                        plt.savefig(path)
                        plt.close()
                        chart_paths['thread_impact_plain'] = path
                        print(f"图表已保存到: {path}")
                    else:
                        print(f"警告: 没有足够数据为普通NTT线程影响图 (N={target_size}, P={target_modulus_plain}) 生成透视表。")
                else:
                    print(f"警告: 没有足够数据绘制普通NTT线程数影响图 (N={target_size}, P={target_modulus_plain})。")
        else:
            print("警告: 没有普通NTT数据可供绘制线程数影响图。")

        crt_ntt_data = data[data['FullImpl'].str.contains('crt_arbitrary')]
        if not crt_ntt_data.empty:
            if crt_ntt_data['Modulus'].nunique() > 1:
                potential_moduli_crt = [m for m in crt_ntt_data['Modulus'].unique() if m != "CRT"]
                if potential_moduli_crt:
                     target_modulus_crt = pd.Series(potential_moduli_crt).mode()[0]
                else:
                     target_modulus_crt = "CRT"
            else:
                target_modulus_crt = crt_ntt_data['Modulus'].unique()[0]

            data_for_thread_plot_crt = crt_ntt_data[
                (crt_ntt_data['FullImpl'].isin(parallel_impls)) &
                (crt_ntt_data['Size'] == target_size) &
                (crt_ntt_data['Modulus'] == target_modulus_crt)
            ]

            if not data_for_thread_plot_crt.empty:
                pivot_data_threads_crt = data_for_thread_plot_crt.pivot_table(index='Threads', columns='FullImpl', values='Time', aggfunc='mean')
                if not pivot_data_threads_crt.empty:
                    pivot_data_threads_crt.plot(kind='line', marker='o', figsize=(12, 7))
                    plt.title(f'不同线程数对CRT-NTT性能的影响 (N={target_size}, P_target={target_modulus_crt})', fontsize=16)
                    plt.xlabel('线程数', fontsize=14)
                    plt.ylabel('平均执行时间 (秒)', fontsize=14)
                    plt.xticks(data_for_thread_plot_crt['Threads'].unique())
                    plt.legend(title='实现方法')
                    plt.grid(True)
                    path = os.path.join(OUTPUT_DIR, f"thread_impact_on_performance_crt_N{target_size}_P{target_modulus_crt}_chinese.png")
                    plt.savefig(path)
                    plt.close()
                    chart_paths['thread_impact_crt'] = path
                    print(f"图表已保存到: {path}")
                else:
                    print(f"警告: 没有足够数据为CRT-NTT线程影响图 (N={target_size}, P_target={target_modulus_crt}) 生成透视表。")

            else:
                print(f"警告: 没有足够数据绘制CRT-NTT线程数影响图 (N={target_size}, P_target={target_modulus_crt})。")
        else:
            print("警告: 没有CRT-NTT数据可供绘制线程数影响图。")

    return chart_paths


def create_chinese_scalability_chart(data):
    if data is None or data.empty:
        print("无数据可供生成可扩展性图表。")
        return {}
        
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    scalability_chart_paths = {}

    parallel_implementations = data[data['Threads'] > 1]['FullImpl'].unique()
    
    if data['Size'].nunique() > 0:
        target_size = data['Size'].quantile(0.5, interpolation='nearest') if data['Size'].nunique() > 1 else data['Size'].unique()[0]
    else:
        print("警告: 可扩展性分析缺少有效的 'Size' 数据。")
        return {}

    impl_categories = {
        "plain": [impl for impl in parallel_implementations if "crt" not in impl],
        "crt": [impl for impl in parallel_implementations if "crt" in impl]
    }

    for category_name, impl_list in impl_categories.items():
        if not impl_list:
            print(f"无 {category_name} 类型的并行实现可供分析。")
            continue

        category_data = data[data['FullImpl'].isin(impl_list) & (data['Size'] == target_size)]
        
        if category_data.empty:
            print(f"无数据用于 {category_name} NTT 在 N={target_size} 时的可扩展性分析。")
            continue
            
        if category_name == "plain":
            unique_moduli_for_plain = category_data['Modulus'].unique()
            numeric_moduli_series = pd.to_numeric(pd.Series(unique_moduli_for_plain), errors='coerce').dropna()
            
            target_modulus = "Unknown" # Default
            if not numeric_moduli_series.empty:
                unique_numeric_moduli = sorted(numeric_moduli_series.unique())
                target_modulus = unique_numeric_moduli[len(unique_numeric_moduli) // 2]
            elif len(unique_moduli_for_plain) > 0:
                # Fallback if no numeric, but other (e.g. string) moduli exist
                modes = category_data['Modulus'].mode()
                if not modes.empty:
                    target_modulus = modes[0]

            if target_modulus == "Unknown":
                print(f"警告: {category_name} NTT 在 N={target_size} 时缺少有效的 'Modulus' 数据，无法生成可扩展性图表。")
                continue # This continue is correctly placed within the for loop over categories

            serial_impl_name_pattern = 'ntt_serial'
            plot_title_suffix = f"Plain NTT (N={target_size}, P={target_modulus})"
            filename_suffix = f"plain_N{target_size}_P{str(target_modulus).replace('.', '_')}"
        else: # CRT
            unique_moduli_for_crt = category_data['Modulus'].unique()
            numeric_crt_moduli = pd.to_numeric(pd.Series([m for m in unique_moduli_for_crt if str(m).upper() != 'CRT']), errors='coerce').dropna().unique()

            target_modulus = "UnknownCRTTarget" # Default
            if len(numeric_crt_moduli) > 0:
                target_modulus = sorted(numeric_crt_moduli)[len(numeric_crt_moduli) // 2]
            elif "CRT" in unique_moduli_for_crt:
                target_modulus = "CRT"
            elif len(unique_moduli_for_crt) > 0:
                modes = category_data['Modulus'].mode()
                if not modes.empty:
                    target_modulus = modes[0]
            
            if target_modulus == "UnknownCRTTarget":
                print(f"警告: {category_name} NTT 在 N={target_size} 时缺少有效的 'Modulus' (P_target) 数据，无法生成可扩展性图表。")
                continue # This continue is correctly placed

            serial_impl_name_pattern = 'ntt_crt_arbitrary_serial'
            plot_title_suffix = f"CRT NTT (N={target_size}, P_target={target_modulus})"
            filename_suffix = f"crt_N{target_size}_P{target_modulus}"

        serial_data = data[
            (data['FullImpl'] == serial_impl_name_pattern) &
            (data['Size'] == target_size) &
            (data['Modulus'] == target_modulus)
        ]
        
        if serial_data.empty:
            print(f"警告: 未找到 {serial_impl_name_pattern} 在 N={target_size}, Modulus/P_target={target_modulus} 下的基准串行时间。可扩展性图表可能不准确或无法生成。")
            approx_serial_times = {}
            for impl in impl_list:
                one_thread_data = category_data[(category_data['FullImpl'] == impl) & (category_data['Threads'] == 1) & (category_data['Modulus'] == target_modulus)]
                if not one_thread_data.empty:
                    approx_serial_times[impl] = one_thread_data['Time'].mean()
            if not approx_serial_times:
                print(f"警告: 无法找到或近似 {category_name} NTT 的基准时间，跳过可扩展性图表生成。")
                continue
            base_time_is_approximate = True
        else:
            base_serial_time = serial_data['Time'].mean()
            base_time_is_approximate = False


        plt.figure(figsize=(14, 7))

        plt.subplot(1, 2, 1)
        for impl in impl_list:
            impl_data = category_data[(category_data['FullImpl'] == impl) & (category_data['Modulus'] == target_modulus)]
            if impl_data.empty:
                print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - impl_data为空，跳过此实现的可扩展性分析(加速比部分)。")
                continue

            current_base_time_for_impl_sp = base_serial_time 
            if base_time_is_approximate:
                if impl not in approx_serial_times:
                    print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - 缺少近似基准时间(加速比部分)。跳过此实现。")
                    continue
                current_base_time_for_impl_sp = approx_serial_times[impl]
            
            # avg_time_by_threads should now be a Series due to upstream column deduplication
            avg_time_by_threads = impl_data.groupby('Threads')['Time'].mean().sort_index()

            if avg_time_by_threads.empty:
                print(f"警告 (效率): Impl={impl}, N={target_size}, P/P_target={target_modulus} - avg_time_by_threads 为空。跳过。")
                continue

            speedup = current_base_time_for_impl_sp / avg_time_by_threads
            
            # 确保speedup数据是一维的
            if isinstance(speedup.values, np.ndarray) and speedup.values.ndim > 1:
                speedup = speedup.iloc[:, 0] if speedup.values.shape[1] > 0 else speedup.iloc[:, -1]
            
            plt.plot(speedup.index, speedup.values, marker='o', linestyle='-', label=impl.replace('ntt_', '').replace('_arbitrary',''))
        
        plt.title(f'加速比 - {plot_title_suffix}{" (近似基准)" if base_time_is_approximate else ""}', fontsize=14)
        plt.xlabel('线程数', fontsize=12)
        plt.ylabel('加速比 (Speedup)', fontsize=12)
        plt.legend()
        plt.grid(True)

        unique_threads = sorted(category_data['Threads'].unique())
        if unique_threads:
             plt.plot(unique_threads, unique_threads, linestyle='--', color='gray', label='理想加速比')
             plt.xticks(unique_threads)

        plt.subplot(1, 2, 2)
        for impl in impl_list:
            impl_data = category_data[(category_data['FullImpl'] == impl) & (category_data['Modulus'] == target_modulus)]
            if impl_data.empty:
                print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - impl_data为空，跳过此实现的可扩展性分析(效率部分)。")
                continue

            current_base_time_for_impl_eff = base_serial_time
            if base_time_is_approximate:
                if impl not in approx_serial_times:
                    print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - 缺少近似基准时间(效率部分)。跳过此实现。")
                    continue
                current_base_time_for_impl_eff = approx_serial_times[impl]
            
            # avg_time_by_threads should now be a Series due to upstream column deduplication
            avg_time_by_threads = impl_data.groupby('Threads')['Time'].mean().sort_index()

            if avg_time_by_threads.empty:
                print(f"警告 (效率): Impl={impl}, N={target_size}, P/P_target={target_modulus} - avg_time_by_threads 为空。跳过。")
                continue
                
            speedup = current_base_time_for_impl_eff / avg_time_by_threads
            
            if speedup.empty or len(speedup.index) == 0:
                print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - speedup 或 speedup.index 为空，跳过效率计算。")
                continue

            # 确保speedup是一维Series
            if isinstance(speedup.values, np.ndarray) and speedup.values.ndim > 1:
                print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - speedup数据是多维的，将展平处理。")
                speedup = speedup.iloc[:, 0] if speedup.values.shape[1] > 0 else speedup.iloc[:, -1]
            
            s_values = speedup.values
            s_index = speedup.index
            divisor_values = s_index.to_numpy(dtype=float)

            if len(s_values) != len(divisor_values):
                print(f"CRITICAL ERROR: Impl={impl}, N={target_size}, P/P_target={target_modulus} - speedup值 ({len(s_values)}) 与索引值 ({len(divisor_values)}) 长度不匹配。跳过效率计算。")
                continue
            
            # 确保s_values是一维数组
            if isinstance(s_values, np.ndarray) and s_values.ndim > 1:
                s_values = s_values.flatten()[:len(divisor_values)]
            
            efficiency_values = np.full_like(s_values, np.nan, dtype=float)
            non_zero_divisor_mask = divisor_values != 0
            
            if np.any(~non_zero_divisor_mask):
                 print(f"警告: Impl={impl}, N={target_size}, P/P_target={target_modulus} - 索引中发现0值线程数。这些点的效率将为NaN。")

            efficiency_values[non_zero_divisor_mask] = s_values[non_zero_divisor_mask] / divisor_values[non_zero_divisor_mask]
            efficiency = pd.Series(efficiency_values, index=s_index)
            
            plt.plot(efficiency.index, efficiency.values, marker='s', linestyle='-', label=impl.replace('ntt_', '').replace('_arbitrary',''))

        plt.title(f'并行效率 - {plot_title_suffix}{" (近似基准)" if base_time_is_approximate else ""}', fontsize=14)
        plt.xlabel('线程数', fontsize=12)
        plt.ylabel('并行效率 (Efficiency)', fontsize=12)
        plt.legend()
        plt.grid(True)
        if unique_threads:
            plt.xticks(unique_threads)
        plt.ylim(0, 1.1)

        plt.tight_layout(pad=3.0)
        fig_path = os.path.join(OUTPUT_DIR, f"scalability_charts_{filename_suffix}_chinese.png")
        plt.savefig(fig_path)
        plt.close()
        scalability_chart_paths[f"scalability_{filename_suffix}"] = fig_path
        print(f"可扩展性图表已保存到: {fig_path}")
        
    return scalability_chart_paths

def generate_detailed_comparison_charts(data):
    if data is None or data.empty:
        print("无数据可供生成详细对比图表。")
        return {}

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    detailed_chart_paths = {}

    for is_crt in [False, True]:
        current_data = data[data['FullImpl'].str.contains('crt_arbitrary') == is_crt].copy()
        if current_data.empty:
            type_str = "CRT" if is_crt else "普通"
            print(f"无{type_str}NTT数据可供生成详细对比图。")
            continue
        
        unique_combinations = current_data[['Modulus', 'Size']].drop_duplicates().to_records(index=False)

        for mod_val, size_val in unique_combinations:
            subset_data = current_data[(current_data['Modulus'] == mod_val) & (current_data['Size'] == size_val)]
            if subset_data.empty:
                continue

            pivot_table = subset_data.pivot_table(index='FullImpl', columns='Threads', values='Time', aggfunc='mean')
            
            if pivot_table.empty:
                print(f"警告: N={size_val}, P={mod_val} 的数据无法生成透视表，跳过此图表。")
                continue

            num_impls = len(pivot_table.index)
            num_threads_groups = len(pivot_table.columns)
            

            pivot_table.plot(kind='bar', figsize=(max(10, num_impls * 1.5), 7), colormap='terrain')
            
            type_str = "CRT" if is_crt else "普通"
            mod_display = f"P_target={mod_val}" if is_crt else f"P={mod_val}"
            plt.title(f'{type_str}NTT性能对比 (N={size_val}, {mod_display})', fontsize=16)
            plt.xlabel('实现方法', fontsize=14)
            plt.ylabel('平均执行时间 (秒)', fontsize=14)
            plt.xticks(rotation=45, ha='right')
            plt.legend(title='线程数', bbox_to_anchor=(1.02, 1), loc='upper left')
            plt.tight_layout()
            
            chart_filename = f"comparison_N{size_val}_P{str(mod_val).replace('.', '_')}_{type_str.lower()}_chinese.png"
            path = os.path.join(OUTPUT_DIR, chart_filename)
            plt.savefig(path)
            plt.close()
            detailed_chart_paths[f"detail_N{size_val}_P{mod_val}_{type_str.lower()}"] = path
            print(f"详细对比图表已保存到: {path}")
            
    return detailed_chart_paths

def generate_markdown_table(df, title=""):
    if df is None or df.empty:
        return f"\\n#### {title}\\n无数据可生成表格。\\n"
    
    header = f"#### {title}\n"
    table_md = df.to_markdown(index=True)
    return header + table_md + "\n"


def generate_chinese_report(data, chart_paths, scalability_chart_paths, detailed_chart_paths):
    if data is None or data.empty:
        return "无性能数据，无法生成报告。"

    report_md = "## 实验结果与可视化分析\n\n"
    report_md += "本章节将展示NTT不同实现的性能测试结果，并通过图表进行可视化分析。\n\n"


    report_md += "### 1. 各种NTT实现的平均性能对比\n"
    report_md += "下图展示了本次测试中所有NTT实现（包括串行、pthread、OpenMP版本及其CRT变体）在所有测试配置下的平均执行时间。这提供了一个关于各种方法总体效率的初步印象。\n"
    if 'avg_performance' in chart_paths:
        report_md += f"![平均性能对比]({os.path.basename(chart_paths['avg_performance'])})\n"
    else:
        report_md += "平均性能对比图表未生成。\n"
    
    avg_performance_series = data.groupby('FullImpl')['Time'].mean()
    if isinstance(avg_performance_series, pd.DataFrame):
        avg_performance_series = avg_performance_series.iloc[:, 0]
    avg_performance_data = avg_performance_series.sort_values(ascending=True)
    if not avg_performance_data.empty:
        best_avg_impl = avg_performance_data.index[0]
        best_avg_time = avg_performance_data.iloc[0]
        report_md += f"从平均性能来看，表现最佳的实现是 **{best_avg_impl}**，其平均执行时间为 {best_avg_time:.4f} 秒。\n\n"

    report_md += "### 2. 问题规模 (N) 对性能的影响\n"
    report_md += "下图比较了不同实现方法在不同问题规模 (N) 下的执行时间。对于并行实现，这里展示的是其在测试中使用的最大线程数下的性能；对于串行实现，则是其自身的性能。这有助于我们理解各种算法的计算复杂度以及它们如何随输入规模扩展。\n"
    if 'size_impact' in chart_paths:
        report_md += f"![规模影响性能]({os.path.basename(chart_paths['size_impact'])})\n"
    else:
        report_md += "问题规模影响图表未生成。\n"
    report_md += "通常，执行时间会随着N的增加而增加。理想情况下，NTT的时间复杂度主要由 $O(N \\log N)$ 的变换主导。\n\n"
    
    report_md += "### 3. 线程数对并行NTT性能的影响\n"
    report_md += "为了评估并行化效果，我们观察了在固定问题规模和模数下，不同线程数对并行NTT（pthread 和 OpenMP）执行时间的影响。\n"
    
    if 'thread_impact_plain' in chart_paths:
        report_md += f"下图展示了普通NTT（非CRT）的并行版本随线程数变化的性能趋势：\n"
        report_md += f"![线程数影响普通NTT性能]({os.path.basename(chart_paths['thread_impact_plain'])})\n"
    else:
        report_md += "普通NTT线程数影响图表未生成。\n"
        
    if 'thread_impact_crt' in chart_paths:
        report_md += f"下图展示了CRT-NTT的并行版本随线程数变化的性能趋势：\n"
        report_md += f"![线程数影响CRT-NTT性能]({os.path.basename(chart_paths['thread_impact_crt'])})\n"
    else:
        report_md += "CRT-NTT线程数影响图表未生成。\n"
    report_md += "理论上，增加线程数可以减少执行时间，但由于并行开销（如线程创建、同步、通信）和任务本身的可并行度限制，性能提升并非线性。\n\n"

    report_md += "### 4. 并行实现的加速比与效率分析\n"
    report_md += "加速比 ($S_p$) 定义为同一任务在单处理器上的执行时间与在 $p$ 个处理器上的执行时间之比。并行效率 ($E_p$) 定义为 $S_p / p$。\n"
    
    plain_scalability_key = [k for k in scalability_chart_paths if "plain" in k]
    if plain_scalability_key:
        report_md += f"下图展示了普通NTT并行实现的加速比和并行效率：\n"
        report_md += f"![普通NTT可扩展性]({os.path.basename(scalability_chart_paths[plain_scalability_key[0]])})\n"
    else:
        report_md += "普通NTT可扩展性图表（加速比和效率）未生成。\n"

    crt_scalability_key = [k for k in scalability_chart_paths if "crt" in k]
    if crt_scalability_key:
        report_md += f"下图展示了CRT-NTT并行实现的加速比和并行效率：\n"
        report_md += f"![CRT-NTT可扩展性]({os.path.basename(scalability_chart_paths[crt_scalability_key[0]])})\n"
    else:
        report_md += "CRT-NTT可扩展性图表（加速比和效率）未生成。\n"
    report_md += "理想情况下，加速比应接近线程数，并行效率接近1。实际中，由于Amhdahl定律以及并行开销，这些值通常会较低。\n\n"
    
    report_md += "### 5. 详细性能数据与对比\n"
    report_md += "为了更细致地分析，以下将展示在特定问题规模N和模数P（或CRT的目标模数P_target）组合下，各实现方法（包括不同线程数）的性能数据表格和对比图。\n\n"
    
    report_md += "#### 原始性能数据摘要 (平均时间)\n"
    summary_table_data = data.groupby(['FullImpl', 'Size', 'Modulus', 'Threads'])['Time'].mean().reset_index()
    summary_table_data.rename(columns={'FullImpl': '实现方法', 'Size': 'N', 'Modulus': 'P/P_target', 'Threads': '线程数', 'Time': '平均时间(s)'}, inplace=True)
    report_md += generate_markdown_table(summary_table_data.head(20), "性能数据选摘 (前20条)") + "\n (完整数据请参考原始CSV文件)\n\n"


    report_md += "#### 特定配置下的性能对比图\n"
    if detailed_chart_paths:
        report_md += "以下图表分别针对不同的 (N, P/P_target) 组合，对比了各种实现（包括不同线程数）的性能：\n"
        for key, path in sorted(detailed_chart_paths.items()):
            try:
                parts = key.split('_')
                n_val = [p for p in parts if p.startswith('N')][0][1:]
                p_val_str = [p for p in parts if p.startswith('P')][0][1:]
                type_val = parts[-1] if parts[-1] in ["plain", "crt"] else "未知类型"
                report_md += f"\n##### 对比: N={n_val}, P/P_target={p_val_str} ({type_val.upper()})\n"
                report_md += f"![详细对比: N={n_val}, P={p_val_str}, 类型={type_val}]({os.path.basename(path)})\n"
            except Exception as e:
                report_md += f"\n##### 详细对比图\n![详细对比图]({os.path.basename(path)})\n (图表描述解析失败: {e})\n"
    else:
        report_md += "详细性能对比图表未生成。\n"
        
    report_md += "\n---\n报告生成完毕。\n"
    return report_md

def main():
    parser = argparse.ArgumentParser(description="从性能数据CSV文件生成图表和Markdown报告")
    parser.add_argument("csv_file", help="包含性能数据的CSV文件路径 (例如: performance_data.csv)")
    args = parser.parse_args()

    print(f"正在从 {args.csv_file} 加载数据...")
    perf_data = load_performance_data(args.csv_file)

    if perf_data is None or perf_data.empty:
        print(f"未能加载或解析数据，或者数据为空。请检查CSV文件: {args.csv_file}")
        return
    
    print("数据加载完毕，开始生成图表...")
    general_chart_paths = create_chinese_performance_charts(perf_data)
    
    scalability_chart_paths = create_chinese_scalability_chart(perf_data)
    
    detailed_chart_paths = generate_detailed_comparison_charts(perf_data)
    print("图表生成完毕.")

    print("开始生成Markdown报告内容...")
    markdown_report_content = generate_chinese_report(perf_data, general_chart_paths, scalability_chart_paths, detailed_chart_paths)
    
    report_filename = "generated_report_content_chinese.md"
    with open(report_filename, "w", encoding="utf-8") as f:
        f.write(markdown_report_content)
    print(f"Markdown报告内容已保存到: {report_filename}")

    print("\\n所有图表已保存在以下目录中:")
    print(f"- 通用性能图表: {', '.join([os.path.basename(p) for p in general_chart_paths.values()])}")
    print(f"- 可扩展性图表: {', '.join([os.path.basename(p) for p in scalability_chart_paths.values()])}")
    print(f"- 详细对比图表: {', '.join([os.path.basename(p) for p in detailed_chart_paths.values()])}")
    print(f"这些图表均位于 '{OUTPUT_DIR}/' 目录下。")

if __name__ == "__main__":
    main() 