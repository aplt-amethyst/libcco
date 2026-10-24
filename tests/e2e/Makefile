# tests/e2e/Makefile
# Standalone Makefile for E2E Test Suite

CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wpedantic
OPT      ?= -O0 -g3
STD      ?= -std=c99
CPPFLAGS += -I../../include -I../../src/internal -I.

TARGET_DIR := ../../target/tests
LIBCCO     := ../../target/debug/liblibcco.a
BIN        := $(TARGET_DIR)/run_e2e

HAS_ASAN := $(shell if [ -f $(LIBCCO) ] && nm $(LIBCCO) 2>/dev/null | grep -q __asan_init; then echo 1; else echo 0; fi)
ifeq ($(HAS_ASAN),1)
CFLAGS  += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

SRCS := e2e_runner.c \
        tier1_features/test_f1_lexer_literals.c \
        tier1_features/test_f2_ast_parser.c \
        tier1_features/test_f3_api_surface.c \
        tier1_features/test_f4_hygiene_diagnostics.c \
        tier2_boundaries/test_tier2_boundaries.c \
        tier3_interactions/test_tier3_interactions.c \
        tier4_scenarios/test_tier4_scenarios.c

.PHONY: all build run clean

all: build run

build: $(BIN)

$(BIN): $(SRCS) $(LIBCCO)
	@mkdir -p $(TARGET_DIR)
	$(CC) $(STD) $(CFLAGS) $(CPPFLAGS) $(OPT) $(SRCS) $(LIBCCO) -lm $(LDFLAGS) -o $@

$(LIBCCO):
	$(MAKE) -C ../.. build.debug

run: $(BIN)
	@cd ../.. && ./target/tests/run_e2e $(ARGS)

clean:
	rm -f $(BIN)
