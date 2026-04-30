CC      = gcc
LEX     = flex
YACC    = bison
CFLAGS  = -Wall -g -I./src/common -I./src/frontend -I./src/backend
LDFLAGS = -lfl

BIN_DIR = bin 
FE_DIR  = src/frontend
BE_DIR  = src/backend
COM_DIR = src/common
TEST_DIR = tests

all: structit

structit: directories $(FE_DIR)/structfe.tab.c $(FE_DIR)/lex.yy.c
	$(CC) $(CFLAGS) $(FE_DIR)/structfe.tab.c $(FE_DIR)/lex.yy.c $(FE_DIR)/codegen.c $(COM_DIR)/ast.c -o $(BIN_DIR)/structit $(LDFLAGS)

$(FE_DIR)/structfe.tab.c $(FE_DIR)/structfe.tab.h: $(FE_DIR)/structfe.y
	$(YACC) -d -o $(FE_DIR)/structfe.tab.c $<

$(FE_DIR)/lex.yy.c: $(FE_DIR)/ANSI-C.l $(FE_DIR)/structfe.tab.h
	$(LEX) -o $@ $<

backend: directories $(BE_DIR)/structbe.tab.c $(BE_DIR)/lex.be.c
	$(CC) $(CFLAGS) $(BE_DIR)/structbe.tab.c $(BE_DIR)/lex.be.c $(COM_DIR)/ast.c -o $(BIN_DIR)/structit_backend $(LDFLAGS)

$(BE_DIR)/structbe.tab.c $(BE_DIR)/structbe.tab.h: $(BE_DIR)/structbe.y
	$(YACC) -d -o $(BE_DIR)/structbe.tab.c $<

$(BE_DIR)/lex.be.c: $(BE_DIR)/ANSI-BE.l $(BE_DIR)/structbe.tab.h
	$(LEX) -o $@ $<

directories:
	@mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)
	rm -f $(FE_DIR)/*.tab.* $(FE_DIR)/lex.yy.c
	rm -f $(BE_DIR)/*.tab.* $(BE_DIR)/lex.be.c
	rm -f $(TEST_DIR)/*__be.c

.PHONY: all backend clean directories
