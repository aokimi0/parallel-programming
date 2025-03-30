#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Set matplotlib to non-interactive backend
import matplotlib.pyplot as plt
import os

# Global font and style settings
plt.rcParams['font.sans-serif'] = ['DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['axes.titlesize'] = 16
plt.rcParams['xtick.labelsize'] = 12
plt.rcParams['ytick.labelsize'] = 12
plt.rcParams['legend.fontsize'] = 12
plt.rcParams['figure.titlesize'] = 18
plt.rcParams['figure.figsize'] = (12, 6)
plt.style.use('seaborn-v0_8-paper')

# Ensure image directory exists
if not os.path.exists('fig'):
    os.makedirs('fig')

# Read data from CSV file
def read_cache_data():
    try:
        # Read Valgrind cache data
        df = pd.read_csv('results/cache_misses.csv')
        # Calculate total cache misses as main metric
        df['misses'] = df['D1_miss'] + df['LLd_miss']
        return df
    except FileNotFoundError:
        print("Warning: cache_misses.csv not found, using sample data")
        # Create sample data for testing
        sizes = [1000, 2000, 3000, 4000, 5000, 7000, 10000]
        methods = ['col', 'row', 'unroll5', 'unroll10', 'unroll15', 'unroll20']
        data = []
        
        for size in sizes:
            # Column access - high cache miss rate
            data.append({'size': size, 'method': 'col', 'I_refs': size*size*10, 'I_miss': size*100, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.09, 'LLd_miss': size*size*0.01, 
                        'instructions': size*size*10, 'misses': size*size*0.1})
            # Row access - medium cache miss rate
            data.append({'size': size, 'method': 'row', 'I_refs': size*size*10, 'I_miss': size*80, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.009, 'LLd_miss': size*size*0.001, 
                        'instructions': size*size*10, 'misses': size*size*0.01})
            # Loop unrolling - gradually decreasing cache miss rate
            data.append({'size': size, 'method': 'unroll5', 'I_refs': size*size*10, 'I_miss': size*70, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.007, 'LLd_miss': size*size*0.001, 
                        'instructions': size*size*10, 'misses': size*size*0.008})
            data.append({'size': size, 'method': 'unroll10', 'I_refs': size*size*10, 'I_miss': size*60, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.006, 'LLd_miss': size*size*0.001, 
                        'instructions': size*size*10, 'misses': size*size*0.007})
            data.append({'size': size, 'method': 'unroll15', 'I_refs': size*size*10, 'I_miss': size*50, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.005, 'LLd_miss': size*size*0.001, 
                        'instructions': size*size*10, 'misses': size*size*0.006})
            data.append({'size': size, 'method': 'unroll20', 'I_refs': size*size*10, 'I_miss': size*40, 
                        'D_refs': size*size*10, 'D1_miss': size*size*0.004, 'LLd_miss': size*size*0.001, 
                        'instructions': size*size*10, 'misses': size*size*0.005})
            
        return pd.DataFrame(data)

# Generate cache miss comparison charts
def plot_cache_misses():
    # Read data
    df = read_cache_data()
    
    # Create multiple figures for different cache analysis aspects
    
    # 1. Total cache misses comparison
    fig1, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 8))
    
    # Set colors and markers for different methods
    colors = {'col': '#8172B3', 'row': '#19A979', 'unroll5': '#FF8C00', 
              'unroll10': '#4E79A7', 'unroll15': '#F28E2B', 'unroll20': '#76B7B2'}
    markers = {'col': 'o', 'row': 's', 'unroll5': 'd', 
               'unroll10': '^', 'unroll15': 'x', 'unroll20': 'P'}
    
    # First chart: All methods cache miss comparison
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        ax1.plot(method_data['size'], method_data['misses'], 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
    
    ax1.set_xlabel('Matrix Size')
    ax1.set_ylabel('Total Cache Misses')
    ax1.set_title('Cache Misses Comparison for Different Methods')
    ax1.set_yscale('log')  # Log scale
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend()
    
    # Second chart: Only unroll methods comparison
    unroll_methods = [m for m in df['method'].unique() if 'unroll' in m]
    for method in unroll_methods:
        method_data = df[df['method'] == method]
        ax2.plot(method_data['size'], method_data['misses'], 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
        
        # Add method labels
        for i, row in method_data.iterrows():
            ax2.annotate(method, (row['size'], row['misses']), 
                        textcoords="offset points", xytext=(0,10), 
                        ha='center', fontsize=10, color='red')
    
    ax2.set_xlabel('Matrix Size')
    ax2.set_ylabel('Total Cache Misses')
    ax2.set_title('Cache Misses Comparison for Loop Unrolling Methods')
    ax2.set_yscale('log')  # Log scale
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend()
    
    plt.tight_layout()
    plt.savefig('fig/cache_misses_comparison.png', dpi=300, bbox_inches='tight')
    plt.close(fig1)
    print("Cache misses comparison chart saved")
    
    # 2. L1 data cache misses and Last Level cache misses comparison
    fig2, (ax3, ax4) = plt.subplots(1, 2, figsize=(20, 8))
    
    # Chart for L1 data cache misses
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        ax3.plot(method_data['size'], method_data['D1_miss'], 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
    
    ax3.set_xlabel('Matrix Size')
    ax3.set_ylabel('L1 Data Cache Misses')
    ax3.set_title('L1 Data Cache Misses Comparison')
    ax3.set_yscale('log')  # Log scale
    ax3.grid(True, linestyle='--', alpha=0.7)
    ax3.legend()
    
    # Chart for Last Level cache misses
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        ax4.plot(method_data['size'], method_data['LLd_miss'], 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
    
    ax4.set_xlabel('Matrix Size')
    ax4.set_ylabel('Last Level Cache Misses')
    ax4.set_title('Last Level Cache Misses Comparison')
    ax4.set_yscale('log')  # Log scale
    ax4.grid(True, linestyle='--', alpha=0.7)
    ax4.legend()
    
    plt.tight_layout()
    plt.savefig('fig/cache_level_misses.png', dpi=300, bbox_inches='tight')
    plt.close(fig2)
    print("Cache level misses comparison chart saved")
    
    # 3. Cache efficiency analysis: misses/references ratio
    fig3, (ax5, ax6) = plt.subplots(1, 2, figsize=(20, 8))
    
    # Calculate cache miss rates
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        # Prevent division by zero
        d1_miss_rate = method_data['D1_miss'] / method_data['D_refs'].replace(0, 1) * 100
        ax5.plot(method_data['size'], d1_miss_rate, 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
    
    ax5.set_xlabel('Matrix Size')
    ax5.set_ylabel('L1 Data Cache Miss Rate (%)')
    ax5.set_title('L1 Data Cache Miss Rate Comparison')
    ax5.grid(True, linestyle='--', alpha=0.7)
    ax5.legend()
    
    # Chart for instruction cache miss rate
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        # Prevent division by zero
        i_miss_rate = method_data['I_miss'] / method_data['I_refs'].replace(0, 1) * 100
        ax6.plot(method_data['size'], i_miss_rate, 
                marker=markers[method], label=method, color=colors[method], 
                linewidth=2, markersize=8)
    
    ax6.set_xlabel('Matrix Size')
    ax6.set_ylabel('Instruction Cache Miss Rate (%)')
    ax6.set_title('Instruction Cache Miss Rate Comparison')
    ax6.grid(True, linestyle='--', alpha=0.7)
    ax6.legend()
    
    plt.tight_layout()
    plt.savefig('fig/cache_miss_rates.png', dpi=300, bbox_inches='tight')
    plt.close(fig3)
    print("Cache miss rates comparison chart saved")

# Read array sum cache analysis data
def read_sum_cache_data():
    try:
        # Read Valgrind cache data
        df = pd.read_csv('results/cache_misses_sum.csv')
        # Calculate total cache misses as main metric
        df['misses'] = df['D1_miss'] + df['LLd_miss']
        return df
    except FileNotFoundError:
        print("Warning: cache_misses_sum.csv not found, using sample data")
        # Create sample data for testing
        methods = ['naive', 'dual', 'recursive']
        data = []
        
        for method in methods:
            if method == 'naive':
                data.append({'method': method, 'I_refs': 2000000, 'I_miss': 2000, 
                            'D_refs': 2000000, 'D1_miss': 200000, 'LLd_miss': 50000, 
                            'instructions': 2000000, 'misses': 250000})
            elif method == 'dual':
                data.append({'method': method, 'I_refs': 2000000, 'I_miss': 1000, 
                            'D_refs': 2000000, 'D1_miss': 100000, 'LLd_miss': 30000, 
                            'instructions': 2000000, 'misses': 130000})
            else:  # recursive
                data.append({'method': method, 'I_refs': 2000000, 'I_miss': 1500, 
                            'D_refs': 2000000, 'D1_miss': 80000, 'LLd_miss': 20000, 
                            'instructions': 2000000, 'misses': 100000})
        
        return pd.DataFrame(data)

# Generate array sum cache analysis charts
def plot_sum_cache_analysis():
    # Read data
    df = read_sum_cache_data()
    
    # Create bar charts
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 8))
    
    # Set colors
    colors = {'naive': '#8172B3', 'dual': '#19A979', 'recursive': '#FF8C00'}
    
    # Draw total cache misses comparison
    methods = df['method'].values
    misses = df['misses'].values
    
    bars1 = ax1.bar(methods, misses, color=[colors[m] for m in methods])
    
    # Add data labels
    for bar in bars1:
        height = bar.get_height()
        ax1.annotate(f'{height:,}',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')
    
    ax1.set_xlabel('Sum Method')
    ax1.set_ylabel('Total Cache Misses')
    ax1.set_title('Total Cache Misses Comparison for Different Sum Methods')
    ax1.grid(True, linestyle='--', alpha=0.3, axis='y')
    
    # Draw L1 and Last Level cache misses comparison
    d1_misses = df['D1_miss'].values
    ll_misses = df['LLd_miss'].values
    
    width = 0.35
    x = np.arange(len(methods))
    
    bars2_1 = ax2.bar(x - width/2, d1_misses, width, label='L1 Data Cache Miss', 
                    color='#4E79A7')
    bars2_2 = ax2.bar(x + width/2, ll_misses, width, label='Last Level Cache Miss', 
                    color='#F28E2B')
    
    # Add data labels
    for bars in [bars2_1, bars2_2]:
        for bar in bars:
            height = bar.get_height()
            ax2.annotate(f'{height:,}',
                        xy=(bar.get_x() + bar.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha='center', va='bottom', fontsize=9)
    
    ax2.set_xlabel('Sum Method')
    ax2.set_ylabel('Cache Misses')
    ax2.set_title('L1 and Last Level Cache Misses Comparison')
    ax2.set_xticks(x)
    ax2.set_xticklabels(methods)
    ax2.legend()
    ax2.grid(True, linestyle='--', alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig('fig/sum_cache_analysis.png', dpi=300, bbox_inches='tight')
    plt.close(fig)
    print("Array sum cache analysis chart saved")

if __name__ == "__main__":
    print("Generating cache miss comparison charts...")
    plot_cache_misses()
    plot_sum_cache_analysis()
    print("All charts generated successfully.") 