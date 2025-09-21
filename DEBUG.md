# Debugging Quick Reference

## Available Debug Configurations

1. **Debug square_map_test** - Main unit tests
2. **Debug algorithms_test** - Binary search merge tests
3. **Debug complexity_test** - Performance and complexity tests
4. **Debug square_map_test (with GTest filter)** - Filtered unit tests
5. **Debug algorithms_test (with GTest filter)** - Filtered merge tests
6. **Debug Current Test (Auto-detect)** - Automatically detects test from current file

## Quick Start

1. Set breakpoints in your code
2. Press `F5` to start debugging
3. Choose a debug configuration
4. For filtered tests, enter a GTest pattern (e.g., `*Insert*`, `SquareMapTest.*`)

## Useful GTest Filters

- `*Insert*` - All insert-related tests
- `*Erase*` - All erase/deletion tests  
- `*Iterator*` - All iterator tests
- `SquareMapTest.*` - All tests in SquareMapTest fixture
- `OrderedMap.Empty` - Just the Empty test
- `MergeMethod.*` - All merge-related tests
- `InjectMethod.*` - All inject-related tests
- `DuplicateRemoval.*` - Duplicate removal tests

## Debug Build

Debug executables are built automatically in `build_debug/` with:
- Debug symbols enabled
- No optimization
- Debug assertions active

## Manual Build Commands

```bash
make debug          # Build all debug executables
make debug-test     # Build and run debug tests
```

## Testing Strategy Insights

- **Always rebuild after changes**: False assumptions can arise from stale builds
- **Use inject-based test setup**: More reliable than dynamic container creation
- **Include check_valid() calls**: Essential for validating container invariants
- **Test incrementally**: Fix one issue at a time with focused test runs

## Debugging Tips

- Use the Debug Console to evaluate expressions during debugging
- Set conditional breakpoints for specific conditions
- Use the Call Stack panel to navigate through function calls
- Check the Variables panel to inspect current state
- Always verify test results after rebuilding
- Use inject() methods for reliable test scenarios
