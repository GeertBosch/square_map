#!/usr/bin/env python3
"""
Script to plot Google Benchmark results in log-log scale.
Usage: python3 plot_benchmarks.py benchmark_results.json
"""

import json
import sys
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from pathlib import Path
import re

def parse_benchmark_name(name):
    """Parse benchmark name to extract components."""
    # Pattern: BM_<operation><map_type, KeyOrder::<order>>/size>
    pattern = r'BM_(\w+)<(.+?)>/(\d+)'
    match = re.match(pattern, name)
    if match:
        operation = match.group(1)
        types = match.group(2).split(', ')
        size = int(match.group(3))
        map_type = types[0] if len(types) > 0 else 'unknown'
        key_order = types[1] if len(types) > 1 else 'unknown'

        # Convert KeyOrder:: format to generator names for compatibility
        if key_order == 'KeyOrder::Sequential':
            generator = 'Sequential'
        elif key_order == 'KeyOrder::Random':
            generator = 'Random'
        else:
            generator = key_order

        return operation, map_type, generator, size
    return None, None, None, None

def load_benchmark_data(json_file):
    """Load and parse benchmark JSON data."""
    with open(json_file, 'r') as f:
        data = json.load(f)

    results = []
    for benchmark in data['benchmarks']:
        name = benchmark['name']
        operation, map_type, generator, size = parse_benchmark_name(name)

        if operation and map_type and generator and size:
            # Extract time_per_item from counters if available
            time_per_item_s = None
            if 'time_per_item' in benchmark:
                time_per_item_s = benchmark['time_per_item']

            results.append({
                'operation': operation,
                'map_type': map_type,
                'generator': generator,
                'size': size,
                'time_per_item_s': time_per_item_s,
                'time_per_item_ns': time_per_item_s * 1000000000.0 if time_per_item_s else None,
                'cpu_time': benchmark['cpu_time'],
                'items_per_second': benchmark.get('items_per_second', 0)
            })

    return pd.DataFrame(results)

def create_plots(df, output_dir='plots'):
    """Create log-log plots for benchmark results."""
    Path(output_dir).mkdir(exist_ok=True)

    # Filter out rows where time_per_item_ns is None
    df = df[df['time_per_item_ns'].notna()]

    if df.empty:
        print("No time_per_item data found in benchmark results!")
        return

    # Set up the plotting style
    plt.style.use('default')
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']

    # Get unique operations
    operations = df['operation'].unique()

    for operation in operations:
        op_data = df[df['operation'] == operation]

        # Create separate plots for each generator type
        generators = op_data['generator'].unique()

        for generator in generators:
            gen_data = op_data[op_data['generator'] == generator]

            plt.figure(figsize=(12, 8))

            # Plot each map type
            map_types = gen_data['map_type'].unique()
            for i, map_type in enumerate(map_types):
                map_data = gen_data[gen_data['map_type'] == map_type].sort_values('size')

                if len(map_data) > 0:
                    plt.loglog(map_data['size'], map_data['time_per_item_ns'],
                              'o-', label=map_type, color=colors[i % len(colors)],
                              linewidth=2, markersize=6)

            plt.xlabel('Container Size', fontsize=12)
            plt.ylabel('Time per Item (nanoseconds)', fontsize=12)
            plt.title(f'{operation} Performance - {generator}', fontsize=14)
            plt.legend(fontsize=10)
            plt.grid(True, alpha=0.3)

            # Add some reference lines for common complexities
            if len(gen_data) > 0:
                sizes = np.array(sorted(gen_data['size'].unique()))
                min_time = gen_data['time_per_item_ns'].min()

                # O(1) reference line
                plt.loglog(sizes, [min_time] * len(sizes), '--',
                          alpha=0.5, color='gray', label='O(1) reference')

                # O(log n) reference line
                log_ref = min_time * np.log2(sizes) / np.log2(sizes[0])
                plt.loglog(sizes, log_ref, '--',
                          alpha=0.5, color='orange', label='O(log n) reference')

                # O(n) reference line
                linear_ref = min_time * sizes / sizes[0]
                plt.loglog(sizes, linear_ref, '--',
                          alpha=0.5, color='red', label='O(n) reference')

            plt.legend(fontsize=10)
            plt.tight_layout()

            # Save the plot
            filename = f"{output_dir}/{operation}_{generator}.png"
            plt.savefig(filename, dpi=300, bbox_inches='tight')
            print(f"Saved plot: {filename}")
            plt.close()

def create_comparison_plot(df, output_dir='plots'):
    """Create a comprehensive comparison plot."""
    # Filter out rows where time_per_item_ns is None
    df = df[df['time_per_item_ns'].notna()]

    if df.empty:
        print("No time_per_item data found for comparison plot!")
        return

    plt.figure(figsize=(16, 12))

    operations = ['Insert', 'Lookup', 'RangeIteration']
    generators = ['Sequential', 'Random']

    for i, operation in enumerate(operations):
        for j, generator in enumerate(generators):
            plt.subplot(len(operations), len(generators), i * len(generators) + j + 1)

            op_data = df[(df['operation'] == operation) & (df['generator'] == generator)]

            if len(op_data) > 0:
                map_types = op_data['map_type'].unique()
                colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']

                for k, map_type in enumerate(map_types):
                    map_data = op_data[op_data['map_type'] == map_type].sort_values('size')
                    if len(map_data) > 0:
                        plt.loglog(map_data['size'], map_data['time_per_item_ns'],
                                  'o-', label=map_type, color=colors[k % len(colors)],
                                  linewidth=1.5, markersize=4)

                plt.xlabel('Size' if i == len(operations) - 1 else '')
                plt.ylabel('Time per Item (ns)' if j == 0 else '')
                plt.title(f'{operation} - {generator}', fontsize=10)
                if i == 0 and j == 0:
                    plt.legend(fontsize=8, bbox_to_anchor=(1.05, 1), loc='upper left')
                plt.grid(True, alpha=0.3)

    plt.tight_layout()
    filename = f"{output_dir}/benchmark_comparison.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"Saved comprehensive comparison: {filename}")
    plt.close()

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_benchmarks.py benchmark_results.json")
        sys.exit(1)

    json_file = sys.argv[1]

    if not Path(json_file).exists():
        print(f"Error: File {json_file} not found")
        sys.exit(1)

    print(f"Loading benchmark data from {json_file}...")
    df = load_benchmark_data(json_file)

    if df.empty:
        print("No valid benchmark data found!")
        sys.exit(1)

    print(f"Found {len(df)} benchmark results")
    print(f"Operations: {', '.join(df['operation'].unique())}")
    print(f"Map types: {', '.join(df['map_type'].unique())}")
    print(f"Generators: {', '.join(df['generator'].unique())}")

    print("Creating plots...")
    create_plots(df)
    create_comparison_plot(df)

    print("Done! Check the 'plots' directory for generated graphs.")

if __name__ == "__main__":
    main()
