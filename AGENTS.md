# AI Agent Guidelines

This document provides guidelines for AI agents working with the `square_map` C++ container library.

## Project Overview

`square_map` is a high-performance associative container that maintains elements in sorted order using a split-range optimization. Key characteristics:
- Template-based design similar to `std::map`
- Split-range internal structure for performance optimization
- Support for erased elements (represented as duplicates across ranges)
- Extract/inject functionality for container manipulation
- Custom merge operations for range consolidation

## Core Components

### Primary Files
- `square_map.h` - Main container implementation
- `square_map_test.cpp` - Comprehensive test suite
- `square_map_bm.cpp` - Benchmark suite
- `algorithms.h` - Stable merge algorithm
- `algorithms_test.cpp` - Unit tests for algorithms
- `README.md` - Theoretical foundation and performance analysis

### Key Methods
- `extract()` - Returns underlying container
- `inject()` - Creates map from container (multiple overloads)
- `merge()` - Public method to merge split ranges
- `split_point()` - Iterator to split between ranges
- Standard container methods: `find()`, `insert()`, `erase()`, etc.

## Development Practices

### Building & Testing
```bash
make debug          # Build debug version
make test           # Run tests
./build_debug/square_map_test --gtest_filter="TestName.*"  # Run specific tests
```

### Test Organization
- **OrderedMap** (8 tests): Basic container operations
- **ShuffledMap** (3 tests): Complex usage patterns
- **EraseMethod** (8 tests): Deletion operations
- **DuplicateRemoval** (1 test): Edge cases with erased elements
- **MergeMethod** (7 tests): Merge functionality
- **InjectMethod** (7 tests): Container reconstruction

### Testing Best Practices
1. **Always rebuild after code changes** - Stale builds cause false assumptions
2. **Use inject-based test setup** - More reliable than dynamic creation
3. **Include `check_valid()` calls** - Validates container invariants
4. **Test incrementally** - Fix one issue at a time

### Coding Style Guidelines
1. **Avoid deeply nested control flow** - Restructure nested if/else chains and loops to be flatter
2. **Use early returns** - Exit functions early when conditions are met to reduce nesting
3. **Use ternary operator `?:`** - Replace simple if/else statements with ternary expressions
4. **Eliminate unnecessary braces** - Remove `{ }` braces around single statements where legally possible
5. **Keep functions short and focused** - Break down complex functions into smaller helper functions
6. **Eliminate duplicate state** - Remove local variables when expressions can be used directly
7. **Factor out helper functions** - Extract common patterns into focused utility functions
8. **Use early continue pattern** - Handle exceptional cases first with `continue` in loops
9. **Merge conditional statements** - Combine multiple early returns into single conditional
10. **Consolidate return conditions** - Group all "return false/true" cases together
11. **Merge conditions into loop headers** - Move loop-termination conditions into the while statement
12. **Delay variable creation** - Create variables only when needed, not speculatively
13. **Recognize state-dependent optimizations** - Split loops when conditions permanently change
14. **Use compact single-line loops** - For simple loops, put everything on one line when readable
15. **Factor predicates from action functions** - Separate pure detection from side-effect operations

### Advanced Refactoring Patterns

#### State-Aware Loop Splitting
Recognize when conditions permanently change and split loops accordingly:
```cpp
// Instead of checking changing condition repeatedly
while (it != end) {
    if (state_changed) {
        // condition becomes permanently true/false here
        handle_differently();
    } else {
        handle_normally();
    }
}

// Split into separate loops
while (it != end && !state_changed) {
    handle_normally();
}
// Now state_changed is permanent
while (it != end) {
    handle_differently();
}
```

#### Delayed Variable Creation
Only create variables when they're actually needed:
```cpp
// Instead of creating speculatively
auto write_it = container.begin();
if (need_compaction) {
    // use write_it
}

// Create only when needed
if (need_compaction) {
    auto write_it = calculate_position();
    // use write_it
}
```

#### Merged Loop Conditions
Move termination conditions into the loop header:
```cpp
// Instead of internal break
while (it != end) {
    if (should_stop(it)) break;
    process(it);
}

// Merge into loop condition
while (it != end && !should_stop(it)) {
    process(it);
}
```

#### Function Decomposition
Break monolithic functions into focused helpers:
```cpp
// Instead of one large function
void complex_operation() {
    // 50 lines of mixed logic
}

// Use focused helper functions
void complex_operation() {
    if (!validate_preconditions()) return;
    process_main_logic();
    cleanup_resources();
}
```

#### Function Decomposition Enabling Better Algorithms
Factor out predicates from action functions to enable cleaner usage patterns:
```cpp
// Combined function forces side effects
bool skip_duplicate(iterator& it) {
    if (!is_duplicate_condition) return false;
    it += 2;  // Side effect
    return true;
}

// Usage requires awkward compensation
while (it != end && !skip_duplicate(it)) ++it;
auto write_it = it - 2;  // Undo the side effect

// Separate predicate enables cleaner algorithms
bool is_duplicate(iterator it) const {
    return duplicate_condition;  // Pure predicate
}

bool skip_duplicate(iterator& it) {
    if (!is_duplicate(it)) return false;
    it += 2;
    return true;
}

// Usage is now clean and direct
while (it != end && !is_duplicate(it)) ++it;
auto write_it = it;  // Perfect positioning
```

#### Early Continue Pattern
Handle exceptional cases first in loops:
```cpp
// Instead of nested conditions
for (auto it = begin; it != end; ++it) {
    if (!is_exceptional_case(*it)) {
        // main processing logic
    }
}

// Use early continue
for (auto it = begin; it != end; ++it) {
    if (is_exceptional_case(*it)) continue;
    // main processing logic (unindented)
}
```

#### Consolidated Conditionals
Merge multiple early returns:
```cpp
// Instead of separate conditions
if (condition1) return false;
if (condition2) return false;
if (condition3) return false;

// Use single conditional
if (condition1 || condition2 || condition3) return false;
```

#### State Variable Elimination
Replace local variables with direct expressions:
```cpp
// Instead of caching values
auto temp_value = expensive_calculation();
if (temp_value > threshold) process(temp_value);

// Use direct access (if not expensive)
if (calculation() > threshold) process(calculation());
```

Examples:
```cpp
// Early returns instead of nesting
if (!condition) return;
if (!other_condition) return;
// do work

// Ternary operator usage
value = condition ? true_case : false_case;

// Remove unnecessary braces
if (condition) return value;

// Compact single-line loops for simple operations
while (it != end && !should_stop(it)) ++it;

// State-dependent loop splitting
while (it != end && !duplicates_found) ++it;  // Fast path
if (it != end) {
    auto write_it = it - offset;  // Create variables when needed
    while (it != end) {
        // Compaction logic
    }
}

// Helper function with consolidated conditionals
bool skip_duplicate(iterator& it) {
    if (at_end(it) || not_equal(it) || different_keys(it)) return false;
    advance_past_duplicate(it);
    return true;
}

// Factored predicate enables better algorithms  
bool is_duplicate(iterator it) const {
    return !at_end(it) && equal_keys(it);
}

bool skip_duplicate(iterator& it) {
    if (!is_duplicate(it)) return false;
    advance_past_duplicate(it);
    return true;
}

// Clean usage without side-effect compensation
while (it != end && !is_duplicate(it)) ++it;
auto write_it = it;  // Perfect positioning
```

An additional guideline is to try and remove duplicate state where possible, as often removing such
duplication can remove redundant operations to update that state. So, consider where you may remove
a local variable and replace the references with an equivalent expression. If the resulting code
reduces the number of state changes, is not likely to affect performance in any meaningful way,
doesn't increase the overall amount of C++ code and doesn't affect readability, that is a good sign
the change is a positive one.

## Agent Guidelines

### Systematic Refactoring Approach
When improving code quality, apply guidelines systematically in this order:

1. **Function decomposition first** - Break large functions into focused helpers
2. **Eliminate duplicate state** - Remove unnecessary local variables  
3. **Apply early returns/continues** - Handle exceptional cases first
4. **Merge conditionals** - Consolidate multiple returns into single statements
5. **Factor common patterns** - Extract repeated logic into helper functions
6. **Test after each step** - Validate functionality is preserved

Use optimistic testing: compile and run all tests after each major refactoring step rather than incremental changes. This approach is more efficient and catches issues early.

### Code Modifications
1. **Read existing tests first** - Understand expected behavior
2. **Read README.md for theoretical context** - Understand complexity analysis and design rationale
3. **Use existing patterns** - Follow established coding style
4. **Validate with tests** - Run relevant test suites after changes
5. **Check container invariants** - Use `check_valid()` helper
6. **Update README.md when necessary** - Keep theoretical analysis current with code changes

### Common Pitfalls
- Split-range structure is complex - understand before modifying
- Erased elements are represented as duplicates across ranges
- Iterator invalidation rules differ from standard containers
- Always consider both left and right range implications

### Testing Approach
- Minimize interactions with the user by using a single command to:
  - Start with changing to the correct directory using an absolute path to prevent errors
  - Rebuild *all* tests programs that are out of date
  - Run *all* tests - don't try to save CPU time by only running a targeted subset
  - Only when there are failures and debugging those, run a specific targeted test with the failure
- By mostly using the same command for testing, I can give you permission to run that autonomously
  saving on user interactions that slow down development

### Debugging Approach
1. Use `DEBUG.md` for debugging configurations
2. Focus on specific test failures with GTest filters
3. Examine container state with `extract()` method
4. Verify split_point behavior in edge cases

### File Modification Strategy
When editing files:
- Include 3-5 lines of context for `replace_string_in_file`
- Test changes incrementally
- Maintain existing code style and patterns
- Update tests when adding new functionality

## Key Insights

### Container Structure
- Elements stored in single vector, logically split into two sorted ranges
- Left range: elements < split_key
- Right range: elements >= split_key
- Erased elements appear in both ranges (making them "invisible")

### Performance Characteristics
- Insert/lookup: O(√n) theoretical complexity (competitive with O(log n) in practice)
- Range operations benefit from split structure
- Merge operations are expensive but amortized
- Real-world performance should be measured with benchmarks rather than theoretical analysis

### Testing Infrastructure
- `inject()` methods enable deterministic test setup
- `check_valid()` ensures container invariants are maintained
- Extract/inject provides round-trip testing capability

## Contact & Contribution

When working on this codebase:
1. Maintain the existing test coverage
2. Follow TDD principles when adding new features
3. Document any new public methods or significant changes
4. Consider performance implications of modifications

For questions about container semantics or implementation details, refer to the comprehensive test
suite which documents expected behavior through executable examples.