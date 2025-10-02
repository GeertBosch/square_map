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
from matplotlib.patches import Rectangle

def add_log_scale_ruler(ax, position='upper right'):
    """Add a logarithmic scale ruler to show time multiplication factors."""
    # Get the current axis limits
    xlim = ax.get_xlim()
    ylim = ax.get_ylim()

    # Calculate ruler dimensions and position in log space
    x_range = np.log10(xlim[1]) - np.log10(xlim[0])
    y_range = np.log10(ylim[1]) - np.log10(ylim[0])

    # Ruler dimensions (as fraction of plot area)
    ruler_width = 0.06 * x_range  # Width for ruler marks

    # Position the ruler
    if position == 'upper right':
        ruler_x = np.log10(xlim[1]) - ruler_width - 0.05 * x_range
        ruler_y_base = np.log10(ylim[1]) - 0.25 * y_range
    elif position == 'upper left':
        ruler_x = np.log10(xlim[0]) + 0.05 * x_range
        ruler_y_base = np.log10(ylim[1]) - 0.25 * y_range
    else:  # lower right
        ruler_x = np.log10(xlim[1]) - ruler_width - 0.05 * x_range
        ruler_y_base = np.log10(ylim[0]) + 0.05 * y_range

    # Convert to linear coordinates
    ruler_x_lin = 10**ruler_x
    base_y = 10**ruler_y_base
    ruler_width_lin = 10**(ruler_x + ruler_width) - ruler_x_lin

    # Draw vertical baseline (main ruler line) - no background box
    baseline_x = ruler_x_lin + ruler_width_lin * 0.1
    ax.plot([baseline_x, baseline_x],
           [base_y, base_y * 5], 'k-', linewidth=1.2, zorder=11)  # Only go to 5x

    # Draw horizontal scale marks with same small size - simplified set
    factors = [1, 2, 5]  # Just the key reference points

    for factor in factors:
        y_pos = base_y * factor
        mark_end = baseline_x + ruler_width_lin * 0.13  # Much smaller tick marks

        # Horizontal tick mark extending right from baseline
        ax.plot([baseline_x, mark_end], [y_pos, y_pos],
               'k-', linewidth=0.8, zorder=11)

        # Label to the right of the mark
        ax.text(mark_end + ruler_width_lin * 0.15, y_pos, f'{factor}×',
               fontsize=7, va='center', ha='left', zorder=12)

    # Title to the right of the ruler
    ax.text(ruler_x_lin + ruler_width_lin * 0.8, base_y * 3,
           'Time\nScale', fontsize=8, ha='left', va='center', weight='bold', zorder=12)

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
    """Load and parse Google Benchmark JSON results."""
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

    # Also return the context information for system details
    context = data.get('context', {})
    return pd.DataFrame(results), context

def format_system_info(context):
    """Format system information for use as plot subtitle."""
    if not context:
        return ""

    # Extract CPU information
    num_cpus = context.get('num_cpus', 0)
    mhz_per_cpu = context.get('mhz_per_cpu', 0)
    cpu_scaling_enabled = context.get('cpu_scaling_enabled', False)

    # Format CPU speed
    if cpu_scaling_enabled or mhz_per_cpu < 1000 or mhz_per_cpu > 10000:
        cpu_speed_str = "???"
    else:
        ghz = mhz_per_cpu / 1000.0
        cpu_speed_str = f"{ghz:.1f} GHz"

    # Extract cache information
    caches = context.get('caches', [])
    l1d_size = None
    l2_size = None

    for cache in caches:
        if cache.get('type') == 'Data' and cache.get('level') == 1:
            l1d_size = cache.get('size')
        elif cache.get('type') == 'Unified' and cache.get('level') == 2:
            l2_size = cache.get('size')

    # Format cache sizes
    def format_cache_size(size_bytes):
        if size_bytes is None:
            return "?"
        if size_bytes < 1024:
            return f"{size_bytes} B"
        elif size_bytes < 1024 * 1024:
            return f"{size_bytes // 1024} KB"
        else:
            return f"{size_bytes // (1024 * 1024)} MB"

    l1d_str = format_cache_size(l1d_size)
    l2_str = format_cache_size(l2_size)

    return f"{num_cpus} × {cpu_speed_str} CPU  -  {l1d_str} L1D  -  {l2_str} L2"

def create_plots(df, context, output_dir='plots'):
    """Create log-log plots for benchmark results."""
    Path(output_dir).mkdir(exist_ok=True)

    # Filter out rows where time_per_item_ns is None
    df = df[df['time_per_item_ns'].notna()]

    if df.empty:
        print("No time_per_item data found in benchmark results!")
        return

    # Set up the plotting style
    plt.style.use('default')

    # Define colors for map types and line styles for operations
    map_type_colors = {
        'square_map_int': '#1f77b4',
        'std_map_int': '#2ca02c',
        'flat_map_int': '#ff7f0e',
        'unordered_map_int': '#d62728',
        'vector_int': '#9467bd',
        'unknown': '#8c564b'
    }

    operation_styles = {
        'Insert': '-',
        'Lookup': ':',
        'RangeIteration': '--'
    }

    # Create combined plots for each generator type
    generators = df['generator'].unique()

    for generator in generators:
        gen_data = df[df['generator'] == generator]

        plt.figure(figsize=(12, 8))

        # Plot each combination of map type and operation
        map_types = gen_data['map_type'].unique()
        operations = gen_data['operation'].unique()

        for map_type in map_types:
            map_color = map_type_colors.get(map_type, '#8c564b')

            for operation in operations:
                op_style = operation_styles.get(operation, '-')

                subset_data = gen_data[
                    (gen_data['map_type'] == map_type) &
                    (gen_data['operation'] == operation)
                ].sort_values('size')

                if len(subset_data) > 0:
                    plt.loglog(subset_data['size'], subset_data['time_per_item_ns'],
                               marker='o', label=f'{map_type} - {operation}',
                              color=map_color, linestyle=op_style,
                              linewidth=2, markersize=6)

        plt.xlabel('Container Size', fontsize=12)
        plt.ylabel('Time per Item (nanoseconds)', fontsize=12)
        plt.title(f'Performance Comparison - {generator}', fontsize=14)
        # Add system info inside the axes, at the top
        system_info = format_system_info(context)
        if system_info:
            plt.gca().text(0.99, 0.98, system_info, fontsize=10, color='dimgray',
                          ha='right', va='top', transform=plt.gca().transAxes, bbox=dict(facecolor='white', alpha=0.7, edgecolor='none', boxstyle='round,pad=0.2'))
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
            log_ref = np.log2(sizes)
            plt.loglog(sizes, log_ref, '--',
                      alpha=0.5, color='orange', label='O(log n) reference')

            # O(n) reference line
            linear_ref = min_time * sizes / sizes[0]
            plt.loglog(sizes, linear_ref, '--',
                      alpha=0.5, color='red', label='O(n) reference')

        # Add logarithmic scale ruler
        add_log_scale_ruler(plt.gca(), 'upper right')

        plt.legend(fontsize=10)
        plt.tight_layout()

        # Save the plot
        filename = f"{output_dir}/combined_{generator}.svg"
        plt.savefig(filename, dpi=300, bbox_inches='tight', metadata={'Date': None})
        print(f"Saved plot: {filename}")
        plt.close()

def create_comparison_plot(df, context, output_dir='plots'):
    """Create a comprehensive comparison plot."""
    # Filter out rows where time_per_item_ns is None
    df = df[df['time_per_item_ns'].notna()]

    if df.empty:
        print("No time_per_item data found for comparison plot!")
        return

    plt.figure(figsize=(16, 12))

    operations = ['Insert', 'Lookup', 'RangeIteration']
    generators = ['Sequential', 'Random']

    # Define colors for map types and line styles for operations
    map_type_colors = {
        'square_map_int': '#1f77b4',
        'std_map_int': '#2ca02c',
        'flat_map_int': '#ff7f0e',
        'unordered_map_int': '#d62728',
        'vector_int': '#9467bd',
        'unknown': '#8c564b'
    }

    operation_styles = {
        'Insert': '-',
        'Lookup': ':',
        'RangeIteration': '--'
    }

    for i, operation in enumerate(operations):
        for j, generator in enumerate(generators):
            ax = plt.subplot(len(operations), len(generators), i * len(generators) + j + 1)
            op_data = df[(df['operation'] == operation) & (df['generator'] == generator)]
            if len(op_data) > 0:
                map_types = op_data['map_type'].unique()
                for map_type in map_types:
                    map_data = op_data[op_data['map_type'] == map_type].sort_values('size')
                    if len(map_data) > 0:
                        color = map_type_colors.get(map_type, '#8c564b')
                        style = operation_styles.get(operation, '-')
                        ax.loglog(map_data['size'], map_data['time_per_item_ns'],
                                  marker='o', label=map_type, color=color, linestyle=style,
                                  linewidth=1.5, markersize=4)
                # Add reference lines for common complexities
                sizes = np.array(sorted(op_data['size'].unique()))
                min_time = op_data['time_per_item_ns'].min()
                ax.loglog(sizes, [min_time] * len(sizes), '--',
                          alpha=0.3, color='gray', linewidth=0.8)
                log_ref = np.log2(sizes)
                ax.loglog(sizes, log_ref, '--',
                          alpha=0.3, color='orange', linewidth=0.8)
                linear_ref = min_time * sizes / sizes[0]
                ax.loglog(sizes, linear_ref, '--',
                          alpha=0.3, color='red', linewidth=0.8)
            # Add scale ruler to first subplot only (to avoid clutter)
            if i == 0 and j == 0:
                add_log_scale_ruler(ax, 'lower right')
            ax.set_xlabel('Size' if i == len(operations) - 1 else '')
            ax.set_ylabel('Time per Item (ns)' if j == 0 else '')
            ax.set_title(f'{operation} - {generator}', fontsize=10)
            if i == 0 and j == 0:
                ax.legend(fontsize=8, bbox_to_anchor=(1.05, 1), loc='upper left')
            ax.grid(True, alpha=0.3)

    plt.tight_layout()
    # Add system info inside each subplot, at the top
    system_info = format_system_info(context)
    if system_info:
        for ax in plt.gcf().get_axes():
            ax.text(0.99, 0.98, system_info, fontsize=10, color='dimgray',
                    ha='right', va='top', transform=ax.transAxes, bbox=dict(facecolor='white', alpha=0.7, edgecolor='none', boxstyle='round,pad=0.2'))
    plt.gcf().suptitle('Benchmark Comparison', fontsize=14)
    filename = f"{output_dir}/benchmark_comparison.svg"
    plt.savefig(filename, dpi=300, bbox_inches='tight', metadata={'Date': None})
    print(f"Saved comprehensive comparison: {filename}")
    plt.close()
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
    df, context = load_benchmark_data(json_file)

    if df.empty:
        print("No valid benchmark data found!")
        sys.exit(1)

    print(f"Found {len(df)} benchmark results")
    print(f"Operations: {', '.join(df['operation'].unique())}")
    print(f"Map types: {', '.join(df['map_type'].unique())}")
    print(f"Generators: {', '.join(df['generator'].unique())}")

    print("Creating plots...")
    create_plots(df, context)
    create_comparison_plot(df, context)

    print("Done! Check the 'plots' directory for generated graphs.")

if __name__ == "__main__":
    main()
