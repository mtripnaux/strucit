CC      = gcc
LEX     = flex
YACC    = bison
CFLAGS  = -Wall -g -I./source/frontend -I./source/backend
LDFLAGS = -lfl

BIN_DIR  = bin
FE_DIR   = source/frontend
BE_DIR   = source/backend
TEST_DIR = tests

all: structit

structit: directories $(FE_DIR)/strucitfe.tab.c $(FE_DIR)/lex.yy.c
	$(CC) $(CFLAGS) \
		$(FE_DIR)/strucitfe.tab.c \
		$(FE_DIR)/lex.yy.c \
		$(FE_DIR)/ast.c \
		$(FE_DIR)/symbol.c \
		$(FE_DIR)/symtable.c \
		$(FE_DIR)/codegen.c \
		$(FE_DIR)/semantic.c \
		-o $(BIN_DIR)/structit $(LDFLAGS)

$(FE_DIR)/strucitfe.tab.c $(FE_DIR)/strucitfe.tab.h: $(FE_DIR)/strucitfe.y
	$(YACC) -d -o $(FE_DIR)/strucitfe.tab.c $<

$(FE_DIR)/lex.yy.c: $(FE_DIR)/ANSI-C.l $(FE_DIR)/strucitfe.tab.h
	$(LEX) -o $@ $<

backend: directories $(BE_DIR)/strucitbe.tab.c $(BE_DIR)/lex.be.c
	$(CC) $(CFLAGS) \
		$(BE_DIR)/strucitbe.tab.c \
		$(BE_DIR)/lex.be.c \
		-o $(BIN_DIR)/structit_backend $(LDFLAGS)

$(BE_DIR)/strucitbe.tab.c $(BE_DIR)/strucitbe.tab.h: $(BE_DIR)/strucitbe.y
	$(YACC) -d -o $(BE_DIR)/strucitbe.tab.c $<

$(BE_DIR)/lex.be.c: $(BE_DIR)/strucitbe.l $(BE_DIR)/strucitbe.tab.h
	$(LEX) -o $@ $<

directories:
	@mkdir -p $(BIN_DIR)

test: structit
	@echo "Running tests..."
	@./scripts/tests.sh

test-validate: structit backend
	@echo "Running tests with backend validation..."
	@./scripts/tests.sh --validate

clean:
	rm -rf $(BIN_DIR)
	rm -f $(FE_DIR)/*.tab.* $(FE_DIR)/lex.yy.c
	rm -f $(BE_DIR)/*.tab.* $(BE_DIR)/lex.be.c
	rm -rf output/*

.PHONY: all backend test test-validate clean directories
