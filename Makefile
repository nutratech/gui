SHELL := /bin/bash
BUILD_DIR := build
CMAKE := cmake
CTEST := ctest
SRC_DIRS := src

# Get version from git
VERSION := $(shell git describe --tags --always 2>/dev/null || echo "v0.0.0")

# Find source files for linting
LINT_LOCS_CPP ?= $(shell git ls-files '*.cpp')
LINT_LOCS_H ?= $(shell git ls-files '*.h')

# Detect number of cores for parallel build
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 1)

.PHONY: config
config:
	$(CMAKE) -E make_directory $(BUILD_DIR)
	$(CMAKE) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DNUTRA_VERSION="$(VERSION)" \
		-B $(BUILD_DIR)

.PHONY: debug
debug: config
	$(CMAKE) --build $(BUILD_DIR) --config Debug --parallel $(NPROC)

.PHONY: release
release:
	$(CMAKE) -E make_directory $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DNUTRA_VERSION="$(VERSION)"
	$(CMAKE) --build $(BUILD_DIR) --config Release --parallel $(NPROC)

.PHONY: appimage
appimage:
	$(CMAKE) -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build build -j$(NPROC)
	$(CMAKE) --build build --target appimage

.PHONY: clean
clean:
	$(CMAKE) -E remove_directory $(BUILD_DIR)


.PHONY: test
test: release
	$(CMAKE) --build $(BUILD_DIR) --target build_tests --config Release
	cd $(BUILD_DIR) && $(CTEST) --output-on-failure -C Release

.PHONY: run
run: debug
	./$(BUILD_DIR)/nutra


.PHONY: format
format:
	-prettier --write .github/
	clang-format -i $(LINT_LOCS_CPP) $(LINT_LOCS_H)


.PHONY: lint
lint: config
	@echo "Linting..."
	@# Build test target first to generate MOC files for tests
	@$(CMAKE) --build $(BUILD_DIR) --target build_tests --config Debug 2>/dev/null || true
	@echo "Running cppcheck..."
	cppcheck --enable=warning,performance,portability \
		--language=c++ --std=c++17 \
		--suppress=missingIncludeSystem \
		-Dslots= -Dsignals= -Demit= -DQT_VERSION_CHECK\(major,minor,patch\)=0 \
		--quiet --error-exitcode=1 \
		$(SRC_DIRS) include tests
	@if [ ! -f $(BUILD_DIR)/compile_commands.json ]; then \
		echo "Error: compile_commands.json not found in $(BUILD_DIR). Run 'make config' first."; \
		exit 1; \
	fi
	@# Create a temp clean compile_commands.json to avoid modifying the original
	@mkdir -p $(BUILD_DIR)/lint_tmp
	@sed 's/-mno-direct-extern-access//g' $(BUILD_DIR)/compile_commands.json > $(BUILD_DIR)/lint_tmp/compile_commands.json
	@echo "Running clang-tidy in parallel..."
	@echo $(LINT_LOCS_CPP) $(LINT_LOCS_H) | xargs -n 1 -P $(NPROC) clang-tidy --quiet -p $(BUILD_DIR)/lint_tmp -extra-arg=-Wno-unknown-argument
	@rm -rf $(BUILD_DIR)/lint_tmp


.PHONY: install
install: release
	@echo "Installing..."
	@$(MAKE) -C $(BUILD_DIR) install || ( \
		$(CMAKE) -DCMAKE_INSTALL_PREFIX=$(HOME)/.local -B $(BUILD_DIR) && \
		$(MAKE) -C $(BUILD_DIR) install \
	)
