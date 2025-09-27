#!/usr/bin/env python3
"""Test script to validate the format_system_info function."""

from format_system_info import format_system_info

# Test cases
test_cases = [
    # Normal case - like the real benchmark data
    {
        "description": "Normal case",
        "context": {
            "num_cpus": 16,
            "mhz_per_cpu": 3600,
            "cpu_scaling_enabled": False,
            "caches": [
                {"type": "Data", "level": 1, "size": 32768},
                {"type": "Unified", "level": 2, "size": 262144}
            ]
        },
        "expected": "16 × 3.6 GHz CPU  -  32 KB L1D  -  256 KB L2"
    },
    
    # CPU scaling enabled
    {
        "description": "CPU scaling enabled",
        "context": {
            "num_cpus": 8,
            "mhz_per_cpu": 2400,
            "cpu_scaling_enabled": True,
            "caches": [
                {"type": "Data", "level": 1, "size": 32768},
                {"type": "Unified", "level": 2, "size": 262144}
            ]
        },
        "expected": "8 × ??? CPU  -  32 KB L1D  -  256 KB L2"
    },
    
    # Low CPU frequency (< 1000 MHz)
    {
        "description": "Low CPU frequency",
        "context": {
            "num_cpus": 4,
            "mhz_per_cpu": 800,
            "cpu_scaling_enabled": False,
            "caches": [
                {"type": "Data", "level": 1, "size": 16384},
                {"type": "Unified", "level": 2, "size": 131072}
            ]
        },
        "expected": "4 × ??? CPU  -  16 KB L1D  -  128 KB L2"
    },
    
    # High CPU frequency (> 10000 MHz)
    {
        "description": "High CPU frequency",
        "context": {
            "num_cpus": 2,
            "mhz_per_cpu": 15000,
            "cpu_scaling_enabled": False,
            "caches": [
                {"type": "Data", "level": 1, "size": 65536},
                {"type": "Unified", "level": 2, "size": 524288}
            ]
        },
        "expected": "2 × ??? CPU  -  64 KB L1D  -  512 KB L2"
    },
    
    # Missing cache information
    {
        "description": "Missing cache info",
        "context": {
            "num_cpus": 6,
            "mhz_per_cpu": 2800,
            "cpu_scaling_enabled": False,
            "caches": []
        },
        "expected": "6 × 2.8 GHz CPU  -  ? L1D  -  ? L2"
    },
    
    # Large cache sizes (MB range)
    {
        "description": "Large cache sizes",
        "context": {
            "num_cpus": 12,
            "mhz_per_cpu": 4200,
            "cpu_scaling_enabled": False,
            "caches": [
                {"type": "Data", "level": 1, "size": 1048576},  # 1 MB
                {"type": "Unified", "level": 2, "size": 8388608}  # 8 MB
            ]
        },
        "expected": "12 × 4.2 GHz CPU  -  1 MB L1D  -  8 MB L2"
    }
]

# Run tests
print("Testing format_system_info function:")
print("=" * 50)

for i, test_case in enumerate(test_cases, 1):
    result = format_system_info(test_case["context"])
    expected = test_case["expected"]
    
    print(f"Test {i}: {test_case['description']}")
    print(f"  Result:   '{result}'")
    print(f"  Expected: '{expected}'")
    print(f"  Status:   {'✅ PASS' if result == expected else '❌ FAIL'}")
    print()

print("Testing complete!")