def format_system_info(context):
    """Format system information for use as plot annotation."""
    if not context:
        return ""
    num_cpus = context.get('num_cpus', 0)
    mhz_per_cpu = context.get('mhz_per_cpu', 0)
    cpu_scaling_enabled = context.get('cpu_scaling_enabled', False)
    if cpu_scaling_enabled or mhz_per_cpu < 1000 or mhz_per_cpu > 10000:
        cpu_speed_str = "???"
    else:
        ghz = mhz_per_cpu / 1000.0
        cpu_speed_str = f"{ghz:.1f} GHz"
    caches = context.get('caches', [])
    l1d_size = None
    l2_size = None
    for cache in caches:
        if cache.get('type') == 'Data' and cache.get('level') == 1:
            l1d_size = cache.get('size')
        elif cache.get('type') == 'Unified' and cache.get('level') == 2:
            l2_size = cache.get('size')
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
