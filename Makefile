# Top-level Makefile for square_map project

# Default target
.DEFAULT_GOAL := all

# Build directory
BUILD_DIR := build

# Default target: ensure build directory exists and run make there
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		cp $(BUILD_DIR)/compile_commands.json . ; \
		echo "Updated compile_commands.json for VSCode"; \
	fi

# Create build directory and generate Makefile if it doesn't exist
$(BUILD_DIR)/Makefile: CMakeLists.txt
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

# Clean target: remove all build files and plot files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@echo "Cleaning plot files..."
	@rm -rf plots/*.png
	@echo "Cleaning test result files..."
	@rm -f test_*.json
	@echo "Cleaning compile commands..."
	@rm -f compile_commands.json
	@echo "Clean complete."

# Force regeneration of build files
rebuild: clean all

# Run tests
test: all
	@$(MAKE) -C $(BUILD_DIR) test

# Run benchmarks
benchmark: all
	@$(MAKE) -C $(BUILD_DIR) square_map_bm
	@cd $(BUILD_DIR) && ./square_map_bm --benchmark_format=json --benchmark_out=benchmark_results.json --benchmark_min_time=0.2s

# Generate plots from benchmark results
plots: benchmark
	@if [ -f $(BUILD_DIR)/benchmark_results.json ]; then \
		source .venv/bin/activate && python3 plot_benchmarks.py $(BUILD_DIR)/benchmark_results.json; \
	else \
		echo "No benchmark results found. Run 'make benchmark' first."; \
	fi

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove all build files, plots, and test results"
	@echo "  rebuild    - Clean and build"
	@echo "  test       - Build and run tests"
	@echo "  benchmark  - Build and run benchmarks"
	@echo "  plots      - Run benchmarks and generate plots"
	@echo "  help       - Show this help message"

.PHONY: all clean rebuild test benchmark plots help
