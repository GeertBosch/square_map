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
	@echo "Cleaning benchmark result files..."
	@rm -f $(BUILD_DIR)/*benchmark*.json
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

# Run quick benchmarks (only square_map with Random key order)
$(BUILD_DIR)/quickbench_results.json: square_map_bm.cpp square_map.h
	@$(MAKE) -C $(BUILD_DIR) square_map_bm
	@cd $(BUILD_DIR) && ./square_map_bm --benchmark_filter="BM_(Insert|Lookup|RangeIteration)<square_map_int, KeyOrder::Random>" --benchmark_out=quickbench_results.json --benchmark_min_time=0.1s

quickbench: $(BUILD_DIR)/quickbench_results.json

# Generate reference benchmarks (std::map and boost::flat_map with Random key order)
$(BUILD_DIR)/quickbench_reference.json: square_map_bm.cpp
	@$(MAKE) -C $(BUILD_DIR) square_map_bm
	@cd $(BUILD_DIR) && ./square_map_bm --benchmark_filter="BM_(Insert|Lookup|RangeIteration)<(std_map_int|flat_map_int), KeyOrder::Random>" --benchmark_out=quickbench_reference.json --benchmark_min_time=0.1s

# Generate quick plot from quickbench results
quickplot: $(BUILD_DIR)/quickbench_results.json $(BUILD_DIR)/quickbench_reference.json
	@if [ -f $(BUILD_DIR)/quickbench_results.json ] && [ -f $(BUILD_DIR)/quickbench_reference.json ]; then \
		source .venv/bin/activate && PLOT_FILE=$$(python3 quickplot.py $(BUILD_DIR)/quickbench_results.json $(BUILD_DIR)/quickbench_reference.json); \
		if [ -n "$$PLOT_FILE" ]; then \
			if command -v timg >/dev/null 2>&1 && ([ "$$TERM_PROGRAM" = "iTerm.app" ] || [ "$$TERM_PROGRAM" = "vscode" ]); then \
				echo "Displaying plot in terminal..."; \
				timg "$$PLOT_FILE"; \
			else \
				echo "Quick plot created: $$PLOT_FILE"; \
			fi; \
		fi; \
	else \
		echo "Missing benchmark results. Run 'make quickbench' first."; \
	fi

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
	@echo "  quickbench - Build and run quick benchmarks (Smallish maps, Random key order)"
	@echo "  quickplot  - Run quickbench and create/display a quick plot"
	@echo "  plots      - Run benchmarks and generate plots"
	@echo "  help       - Show this help message"

.PHONY: all clean rebuild test benchmark quickbench quickplot plots help
