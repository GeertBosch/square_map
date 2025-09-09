#!/usr/bin/env python3
"""
Script to create a quick plot from square_map benchmark results with reference implementations.
Usage: python3 quickplot.py quickbench_results.json [quickbench_reference.json]
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
        return operation, map_type, key_order, size
    return None, None, None, None

def load_benchmark_data(json_file):
    """Load and parse benchmark JSON data."""
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    results = []
    for benchmark in data['benchmarks']:
        name = benchmark['name']
        operation, map_type, key_order, size = parse_benchmark_name(name)
        
        if operation and map_type and key_order and size:
            # Extract time_per_item from counters if available
            time_per_item_s = benchmark.get('time_per_item', None)
            
            results.append({
                'operation': operation,
                'map_type': map_type,
                'key_order': key_order,
                'size': size,
                'time_per_item_s': time_per_item_s,
                'time_per_item_ns': time_per_item_s * 1000000000.0 if time_per_item_s else None,
                'cpu_time': benchmark['cpu_time'],
                'items_per_second': benchmark.get('items_per_second', 0)
            })
    
    return pd.DataFrame(results)

def create_quickplot(square_map_df, reference_df=None, output_file='plots/quickplot.png'):
    """Create a single plot showing all operations with optional reference implementations."""
    # Filter out rows where time_per_item_ns is None
    square_map_df = square_map_df[square_map_df['time_per_item_ns'].notna()]
    
    if square_map_df.empty:
        print("No time_per_item data found in square_map benchmark results!")
        return False
    
    # Set up the plotting style
    plt.style.use('default')
    plt.figure(figsize=(12, 8))
    
    # Define colors for map types and line styles for operations
    map_type_colors = {
        'square_map_int': '#1f77b4',
        'std_map_int': '#2ca02c', 
        'flat_map_int': '#ff7f0e',
        'unordered_map_int': '#d62728',
        'vector_int': '#9467bd'
    }
    
    operation_styles = {
        'Insert': '-',
        'Lookup': '--',
        'RangeIteration': ':'
    }
    
    operation_markers = {
        'Insert': 'o',
        'Lookup': '^',
        'RangeIteration': 's'
    }
    
    operations = ['Insert', 'Lookup', 'RangeIteration']
    
    # Plot square_map results first
    for operation in operations:
        op_data = square_map_df[square_map_df['operation'] == operation].sort_values('size')
        
        if len(op_data) > 0:
            color = map_type_colors.get('square_map_int', '#1f77b4')
            style = operation_styles.get(operation, '-')
            marker = operation_markers.get(operation, 'o')
            
            plt.loglog(op_data['size'], op_data['time_per_item_ns'], 
                      marker=marker, label=f'{operation} (square_map)', color=color,
                      linewidth=2, markersize=6, linestyle=style)
    
    # Plot reference implementations if provided
    if reference_df is not None and not reference_df.empty:
        reference_df = reference_df[reference_df['time_per_item_ns'].notna()]
        
        # Plot std::map results
        for operation in operations:
            std_map_data = reference_df[
                (reference_df['operation'] == operation) & 
                (reference_df['map_type'] == 'std_map_int')
            ].sort_values('size')
            
            if len(std_map_data) > 0:
                color = map_type_colors.get('std_map_int', '#ff7f0e')
                style = operation_styles.get(operation, '-')
                marker = operation_markers.get(operation, 'o')
                
                plt.loglog(std_map_data['size'], std_map_data['time_per_item_ns'], 
                          marker=marker, label=f'{operation} (std::map)', color=color,
                          linewidth=2, markersize=5, linestyle=style, alpha=0.8)
        
        # Plot boost::flat_map results
        for operation in operations:
            flat_map_data = reference_df[
                (reference_df['operation'] == operation) & 
                (reference_df['map_type'] == 'flat_map_int')
            ].sort_values('size')
            
            if len(flat_map_data) > 0:
                color = map_type_colors.get('flat_map_int', '#2ca02c')
                style = operation_styles.get(operation, '-')
                marker = operation_markers.get(operation, 'o')
                
                plt.loglog(flat_map_data['size'], flat_map_data['time_per_item_ns'], 
                          marker=marker, label=f'{operation} (boost::flat_map)', color=color,
                          linewidth=2, markersize=5, linestyle=style, alpha=0.8)
    
    plt.xlabel('Container Size', fontsize=12)
    plt.ylabel('Time per Item (nanoseconds)', fontsize=12)
    plt.title('Map Performance Comparison - Random Key Order', fontsize=14)
    plt.legend(fontsize=9, loc='upper left')
    plt.grid(True, alpha=0.3)
    
    # Add reference lines for common complexities
    all_data = square_map_df
    if reference_df is not None:
        all_data = pd.concat([square_map_df, reference_df], ignore_index=True)
    
    if len(all_data) > 0:
        sizes = np.array(sorted(all_data['size'].unique()))
        min_time = all_data['time_per_item_ns'].min()
        
        # O(1) reference line
        plt.loglog(sizes, [min_time] * len(sizes), '--', 
                  alpha=0.4, color='gray', label='O(1) reference', linewidth=1)
        
        # O(log n) reference line  
        log_ref = min_time * np.log2(sizes) / np.log2(sizes[0])
        plt.loglog(sizes, log_ref, '--', 
                  alpha=0.4, color='orange', label='O(log n) reference', linewidth=1)
    
    plt.legend(fontsize=9, loc='upper left')
    plt.tight_layout()
    
    # Ensure plots directory exists
    Path('plots').mkdir(exist_ok=True)
    
    # Save the plot
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    
    return True

def main():
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Usage: python3 quickplot.py quickbench_results.json [quickbench_reference.json]")
        sys.exit(1)
    
    square_map_file = sys.argv[1]
    reference_file = sys.argv[2] if len(sys.argv) == 3 else None
    
    if not Path(square_map_file).exists():
        print(f"Error: File {square_map_file} not found")
        sys.exit(1)
    
    square_map_df = load_benchmark_data(square_map_file)
    
    if square_map_df.empty:
        print("No valid benchmark data found in square_map results!")
        sys.exit(1)
    
    reference_df = None
    if reference_file and Path(reference_file).exists():
        reference_df = load_benchmark_data(reference_file)
    
    if create_quickplot(square_map_df, reference_df):
        print("plots/quickplot.png")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
