# ============================================================================
#  Generic Makefile
#  Usage: make help
# ============================================================================

SHELL := /bin/bash

# --- project ----------------------------------------------------------------
PROJECT     ?= $(notdir $(CURDIR))
DESCRIPTION ?= $(PROJECT)
SRC_DIR     ?= src
TEST_DIR    ?= tests
INC_DIR     ?= include
EXAMPLE_DIR ?= examples
BUILD_DIR   ?= target
DOCS_DIR    ?= docs
DOXYFILE    ?= Doxyfile
BIN         ?= $(PROJECT)

# --- install ----------------------------------------------------------------
PREFIX  ?= /usr/local
DESTDIR ?=

# --- parameter catch-all ----------------------------------------------------
CUSTOM_CFLAGS :=
CUSTOM_CPPFLAGS :=

# Check if help is requested anywhere in goals
ifneq ($(filter -h --help help,$(MAKECMDGOALS)),)
  .DEFAULT_GOAL := help
  # Make all other goals do nothing
  $(eval %:: ; @:)
else
  ifneq ($(MAKECMDGOALS),)
    # Turn all extra goals into do-nothing targets
    $(eval %:: ; @:)

    # Extract options using a shell loop
    PARSE_OUTPUT := $(shell \
        args="$(MAKECMDGOALS)"; \
        mode="none"; \
        for arg in $$args; do \
            if [ "$$arg" = "-co" ] || [ "$$arg" = "--compile-option" ]; then \
                mode="co"; \
            elif [ "$$arg" = "-fo" ] || [ "$$arg" = "--feature-option" ]; then \
                mode="fo"; \
            else \
                if [ "$$mode" = "co" ]; then \
                    echo "CUSTOM_CFLAGS+=$$arg"; \
                elif [ "$$mode" = "fo" ]; then \
                    echo "CUSTOM_CPPFLAGS+=-D$$arg"; \
                fi; \
            fi; \
        done \
    )
    $(eval $(PARSE_OUTPUT))
  endif
endif

# --- toolchain --------------------------------------------------------------
CC       ?= cc
CXX      ?= c++
AR       ?= ar
NM       ?= nm
CSTD     ?= c11
CXXSTD   ?= c++17
WARNING_FLAGS ?= -Wall -Wextra -Wpedantic
CFLAGS   ?= $(CUSTOM_CFLAGS)
CXXFLAGS ?= $(CUSTOM_CFLAGS)
CPPFLAGS += -I$(INC_DIR) $(CUSTOM_CPPFLAGS)
LDFLAGS  ?=
LDLIBS   ?=

ifneq ($(SANITIZER),)
CFLAGS   += -fsanitize=$(SANITIZER)
CXXFLAGS += -fsanitize=$(SANITIZER)
LDFLAGS  += -fsanitize=$(SANITIZER)
endif

OPT_DEBUG   ?= -O0 -g3 -DDEBUG
OPT_RELEASE ?= -O2 -DNDEBUG

# --- ansi -------------------------------------------------------------------
RESET  := \033[0m
BOLD   := \033[1m
DIM    := \033[2m
CYAN   := \033[36m
GREEN  := \033[32m
YELLOW := \033[33m
RED    := \033[31m

STEP = printf '$(CYAN)$(BOLD)==>$(RESET) %s\n'
OK   = printf '$(GREEN)$(BOLD)  ok$(RESET) %s\n'
WARN = printf '$(YELLOW)warn:$(RESET) %s\n'
ERR  = printf '$(RED)$(BOLD)error:$(RESET) %s\n'

# --- sources ----------------------------------------------------------------
SRCS      := $(shell find $(SRC_DIR) -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) 2>/dev/null | sort)
MAIN_SRCS := $(wildcard $(SRC_DIR)/main.c $(SRC_DIR)/main.cc $(SRC_DIR)/main.cpp $(SRC_DIR)/main.cxx)
LIB_SRCS  := $(filter-out $(MAIN_SRCS),$(SRCS))
CXX_SRCS  := $(filter %.cc %.cpp %.cxx,$(SRCS))

IS_LIBRARY := $(if $(MAIN_SRCS),0,1)
LD         := $(if $(CXX_SRCS),$(CXX),$(CC))

ifeq ($(IS_LIBRARY),1)
override CFLAGS   += -fPIC -fvisibility=hidden -DCCO_EXPORTS
override CXXFLAGS += -fPIC -fvisibility=hidden -DCCO_EXPORTS
endif

MODE ?= debug
ifeq ($(MODE),release)
OPT := $(OPT_RELEASE)
else
OPT := $(OPT_DEBUG)
endif

OBJ_DIR := $(BUILD_DIR)/$(MODE)/obj
OBJS    := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

TARGET_OF        = $(BUILD_DIR)/$(1)/$(if $(filter 1,$(IS_LIBRARY)),lib$(BIN).a,$(BIN))
SHARED_TARGET_OF = $(BUILD_DIR)/$(1)/$(if $(filter 1,$(IS_LIBRARY)),libcco.so,lib$(BIN).so)

TARGET        := $(call TARGET_OF,$(MODE))
STATIC_TARGET := $(TARGET)
SHARED_TARGET := $(call SHARED_TARGET_OF,$(MODE))

# --- object rules -----------------------------------------------------------
$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(STEP) "$<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNING_FLAGS) -std=$(CSTD) $(OPT) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.cc.o: $(SRC_DIR)/%.cc
	@mkdir -p $(dir $@)
	@$(STEP) "$<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(WARNING_FLAGS) -std=$(CXXSTD) $(OPT) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(STEP) "$<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(WARNING_FLAGS) -std=$(CXXSTD) $(OPT) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.cxx.o: $(SRC_DIR)/%.cxx
	@mkdir -p $(dir $@)
	@$(STEP) "$<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(WARNING_FLAGS) -std=$(CXXSTD) $(OPT) -MMD -MP -c $< -o $@

ifeq ($(IS_LIBRARY),1)
$(STATIC_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@$(STEP) "link static $@"
	@$(AR) rcs $@ $^

$(SHARED_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@$(STEP) "link shared $@"
	@$(CC) -shared -o $@ $^ $(LDFLAGS) -lm
else
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@$(STEP) "link $@"
	@$(LD) $^ $(LDFLAGS) $(LDLIBS) -o $@
endif

-include $(DEPS)

# ============================================================================
#  Targets
# ============================================================================

.DEFAULT_GOAL := help
.PHONY: all help build build.debug build.release run examples tests clean docs lint install uninstall check-symbols benchmarks benchmark

##@ General

help: ## Show this help
	@printf '\n'
	@printf '  $(BOLD)%s$(RESET)\n' '$(PROJECT)'
	@printf '  $(DIM)%s$(RESET)\n' '$(DESCRIPTION)'
	@awk 'BEGIN {FS = ":.*##"} \
		/^##@/ { printf "\n\033[1;33m  %s\033[0m\n", substr($$0, 5); next } \
		/^[a-zA-Z0-9_.-]+:.*?##/ { \
			msg = $$2; sub(/^[ \t]*/, "", msg); \
			printf "  \033[36m%-22s\033[0m \033[2m%s\033[0m\n", $$1, msg \
		}' $(MAKEFILE_LIST)
	@printf '\n  \033[1;33mOptions\033[0m\n'
	@printf '  \033[36m-co, --compile-option\033[0m  \033[2mCustom compile options (e.g. -O3)\033[0m\n'
	@printf '  \033[36m-fo, --feature-option\033[0m  \033[2mCustom feature macros (e.g. CCO_ENABLE_EVAL)\033[0m\n'
	@printf '  \033[36m-h, --help           \033[0m  \033[2mShow this help (same as make help)\033[0m\n'
	@printf '\n'

##@ Build

all: $(if $(filter 1,$(IS_LIBRARY)),$(STATIC_TARGET) $(SHARED_TARGET),$(TARGET)) ## Build all library targets (static and shared)
	@$(OK) "built targets for MODE=$(MODE)"

build: build.debug ## Build debug binary (alias of build.debug)

build.debug: ## Compile with debug flags
	@$(MAKE) --no-print-directory all MODE=debug

build.release: ## Compile with release flags
	@$(MAKE) --no-print-directory all MODE=release

##@ Run

run: build.debug ## Run the debug binary (ARGS="..." to pass arguments)
	@if [ "$(IS_LIBRARY)" = "1" ]; then \
		$(WARN) "library project, nothing to run"; \
	else \
		$(STEP) "run $(BIN)"; \
		$(BUILD_DIR)/debug/$(BIN) $(ARGS); \
	fi

examples: build.debug ## Build one example: make examples name=<file>
	@if [ -z "$(name)" ]; then \
		printf 'usage: make examples name=<file>\n\n'; \
		printf 'available:\n'; \
		find $(EXAMPLE_DIR) -maxdepth 1 -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) 2>/dev/null \
			| sed 's|.*/||; s|\.[^.]*$$||' | sort | sed 's|^|  |'; \
		exit 0; \
	fi; \
	src=$$(ls $(EXAMPLE_DIR)/$(name).c $(EXAMPLE_DIR)/$(name).cc $(EXAMPLE_DIR)/$(name).cpp 2>/dev/null | head -n1); \
	if [ -z "$$src" ]; then $(ERR) "no such example: $(name)"; exit 1; fi; \
	comp="$(CC)"; case "$$src" in *.c) ;; *) comp="$(CXX)";; esac; \
	mkdir -p $(BUILD_DIR)/examples; \
	$(STEP) "build $$src"; \
	$$comp $(CPPFLAGS) -DCCO_STATIC $(WARNING_FLAGS) $(OPT) $$src $(BUILD_DIR)/debug/liblibcco.a $(LDFLAGS) $(LDLIBS) -o $(BUILD_DIR)/examples/$(name) || { $(ERR) "example build failed"; exit 1; }; \
	$(OK) "$(BUILD_DIR)/examples/$(name)"

##@ Quality

check-symbols: $(SHARED_TARGET) ## Validate exported symbols in shared library
	@$(STEP) "checking exported symbols in $(SHARED_TARGET)"
	@if ! command -v $(NM) >/dev/null 2>&1; then \
		$(ERR) "$(NM) not found"; exit 1; \
	fi
	@symbols=$$($(NM) -D --defined-only $(SHARED_TARGET) 2>/dev/null | awk '{print $$NF}' | sort); \
	count=$$(echo "$$symbols" | grep -c . || true); \
	if [ "$$count" -ne 21 ]; then \
		$(ERR) "expected exactly 21 exported symbols, found $$count"; \
		echo "$$symbols"; \
		exit 1; \
	fi; \
	expected="cco_arena_free cco_arena_new cco_arena_reset cco_array_get_count cco_array_get_item cco_clear_diagnostics cco_get_last_error cco_map_get_by_key cco_map_get_count cco_map_get_key cco_map_get_value cco_object_get_boolean cco_object_get_float cco_object_get_integer cco_object_get_string cco_object_get_type cco_object_release cco_object_retain cco_parse_options_init cco_parse_string cco_serialize_to_string"; \
	for sym in $$expected; do \
		if ! echo "$$symbols" | grep -qx "$$sym"; then \
			$(ERR) "missing expected symbol: $$sym"; \
			exit 1; \
		fi; \
	done; \
	for sym in $$symbols; do \
		if ! echo " $$expected " | grep -q " $$sym "; then \
			$(ERR) "leaked unexpected symbol: $$sym"; \
			exit 1; \
		fi; \
	done; \
	$(OK) "verified exactly 21 public symbols and 0 internal leaks in $(SHARED_TARGET)"

tests: build.debug ## Build and run the test suite
	@if [ -f $(TEST_DIR)/Makefile ]; then \
		$(STEP) "delegate to $(TEST_DIR)/Makefile"; \
		$(MAKE) -C $(TEST_DIR) CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"; \
	elif [ -n "$$(find $(TEST_DIR) -maxdepth 1 -name '*.c' -o -name '*.cc' -o -name '*.cpp' 2>/dev/null | head -n1)" ]; then \
		mkdir -p $(BUILD_DIR)/debug/tests; \
		$(STEP) "build tests"; \
		$(LD) $(CPPFLAGS) $(WARNING_FLAGS) $(OPT) $$(find $(TEST_DIR) -maxdepth 1 -name '*.c' -o -name '*.cc' -o -name '*.cpp') $(LIB_SRCS) $(LDFLAGS) $(LDLIBS) -o $(BUILD_DIR)/debug/tests/run || \
		{ $(ERR) "test build failed"; exit 1; }; \
		$(STEP) "run tests"; \
		$(BUILD_DIR)/debug/tests/run; \
	else \
		$(WARN) "no tests found in $(TEST_DIR)/"; \
	fi

coverage: ## Run tests with gcov code coverage enabled
	@$(STEP) "clean build before coverage"
	@$(MAKE) clean >/dev/null
	@$(STEP) "build tests with coverage"
	@$(MAKE) tests OPT="-O0 -g3" CFLAGS="$(CFLAGS) -fprofile-arcs -ftest-coverage" LDFLAGS="$(LDFLAGS) -fprofile-arcs -ftest-coverage"
	@$(STEP) "generate coverage report"
	@mkdir -p target/coverage
	@for f in src/internal/*.c; do \
		gcov -o target/debug/obj/internal/$$(basename $$f).gcno $$f >/dev/null 2>&1 || true; \
	done
	@mv *.gcov target/coverage/ 2>/dev/null || true
	@$(OK) "coverage report generated in target/coverage/"

lint: ## Run static analysis on the sources
	@if command -v cppcheck >/dev/null 2>&1; then \
		$(STEP) "cppcheck"; \
		cppcheck --enable=warning,style,performance --inline-suppr --quiet $(SRCS) || exit 1; \
		$(OK) "cppcheck clean"; \
	elif command -v clang-tidy >/dev/null 2>&1; then \
		$(STEP) "clang-tidy"; \
		clang-tidy $(SRCS) -- $(CPPFLAGS) || exit 1; \
		$(OK) "clang-tidy clean"; \
	else \
		$(ERR) "no linter found (install cppcheck or clang-tidy)"; exit 1; \
	fi

##@ Benchmarks

BENCH_DIR   := benchmarks
BENCH_BUILD := $(BUILD_DIR)/release/benchmarks

$(BENCH_BUILD)/bench_parse: $(BENCH_DIR)/bench_parse.c $(call TARGET_OF,release)
	@mkdir -p $(BENCH_BUILD)
	@$(STEP) "build bench_parse"
	@$(CC) $(CPPFLAGS) -DCCO_STATIC $(WARNING_FLAGS) -std=$(CSTD) $(OPT_RELEASE) $< $(call TARGET_OF,release) $(LDFLAGS) -lm -o $@

$(BENCH_BUILD)/bench_serialize: $(BENCH_DIR)/bench_serialize.c $(call TARGET_OF,release)
	@mkdir -p $(BENCH_BUILD)
	@$(STEP) "build bench_serialize"
	@$(CC) $(CPPFLAGS) -DCCO_STATIC $(WARNING_FLAGS) -std=$(CSTD) $(OPT_RELEASE) $< $(call TARGET_OF,release) $(LDFLAGS) -lm -o $@

benchmarks: build.release $(BENCH_BUILD)/bench_parse $(BENCH_BUILD)/bench_serialize ## Build and run benchmark suite
	@$(STEP) "run bench_parse"
	@$(BENCH_BUILD)/bench_parse $(ARGS)
	@$(STEP) "run bench_serialize"
	@$(BENCH_BUILD)/bench_serialize $(ARGS)
	@$(OK) "benchmarks complete"

benchmark: benchmarks ## Alias of benchmarks

##@ Documentation

docs: ## Generate API documentation with Doxygen
	@if ! command -v doxygen >/dev/null 2>&1; then $(ERR) "doxygen not found"; exit 1; fi; \
	if [ ! -f $(DOXYFILE) ]; then $(ERR) "$(DOXYFILE) missing, run: doxygen -g $(DOXYFILE)"; exit 1; fi; \
	$(STEP) "doxygen $(DOXYFILE)"; \
	doxygen $(DOXYFILE) >/dev/null; \
	$(OK) "$(DOCS_DIR)/html/index.html"

##@ Install

install: build.release ## Install artifacts under $(DESTDIR)$(PREFIX)
	@if [ "$(IS_LIBRARY)" = "1" ]; then \
		install -d $(DESTDIR)$(PREFIX)/lib; \
		install -m 644 $(call TARGET_OF,release) $(DESTDIR)$(PREFIX)/lib/; \
		install -m 755 $(call SHARED_TARGET_OF,release) $(DESTDIR)$(PREFIX)/lib/; \
		$(OK) "$(DESTDIR)$(PREFIX)/lib/$(notdir $(call TARGET_OF,release))"; \
		$(OK) "$(DESTDIR)$(PREFIX)/lib/$(notdir $(call SHARED_TARGET_OF,release))"; \
	else \
		install -d $(DESTDIR)$(PREFIX)/bin; \
		install -m 755 $(call TARGET_OF,release) $(DESTDIR)$(PREFIX)/bin/$(BIN); \
		$(OK) "$(DESTDIR)$(PREFIX)/bin/$(BIN)"; \
	fi
	@if [ -d $(INC_DIR) ]; then \
		install -d $(DESTDIR)$(PREFIX)/include/$(PROJECT); \
		cp -R $(INC_DIR)/. $(DESTDIR)$(PREFIX)/include/$(PROJECT)/; \
		$(OK) "$(DESTDIR)$(PREFIX)/include/$(PROJECT)"; \
	fi

uninstall: ## Remove installed artifacts
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	@rm -f $(DESTDIR)$(PREFIX)/lib/lib$(BIN).a
	@rm -f $(DESTDIR)$(PREFIX)/lib/libcco.so
	@rm -rf $(DESTDIR)$(PREFIX)/include/$(PROJECT)
	@$(OK) "uninstalled $(PROJECT)"

##@ Clean

clean: ## Remove all build artifacts
	@$(STEP) "remove $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)
	@$(OK) "clean"
