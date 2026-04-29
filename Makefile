CC      = gcc
LEX     = flex
YACC    = bison
MKDIR   = mkdir -p

CFLAGS  = -Wall -Wextra -g -I./src/common -I./src/frontend -I./src/backend
LDFLAGS = -lfl

SRC_DIR     = source
COMMON_DIR  = $(SRC_DIR)/common
FRONT_DIR      = $(SRC_DIR)/frontend
BACK_DIR      = $(SRC_DIR)/backend
BIN_DIR     = bin
OBJ_DIR     = obj

FE_TARGET = $(BIN_DIR)/structfe
BE_TARGET = $(BIN_DIR)/structbe

all: directories $(FE_TARGET) $(BE_TARGET)

directories:
	@$(MKDIR) $(BIN_DIR)
	@$(MKDIR) $(OBJ_DIR)

$(FE_TARGET): $(OBJ_DIR)/codegen.o $(OBJ_DIR)/ast.o $(OBJ_DIR)/structfe.tab.o $(OBJ_DIR)/lex.yy.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(FE_DIR)/structfe.tab.c $(FE_DIR)/structfe.tab.h: $(FE_DIR)/structfe.y
	$(YACC) -d -o $(FE_DIR)/structfe.tab.c $<

$(FE_DIR)/lex.yy.c: $(FE_DIR)/ANSI-C.l $(FE_DIR)/structfe.tab.h
	$(LEX) -o $@ $<

$(BE_TARGET): $(OBJ_DIR)/ast.o $(OBJ_DIR)/structbe.tab.o $(OBJ_DIR)/lex.be.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BE_DIR)/structbe.tab.c $(BE_DIR)/structbe.tab.h: $(BE_DIR)/structbe.y
	$(YACC) -d -o $(BE_DIR)/structbe.tab.c $<

$(BE_DIR)/lex.be.c: $(BE_DIR)/ANSI-BE.l $(BE_DIR)/structbe.tab.h
	$(LEX) -o $@ $<

$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(FE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(FE_DIR)/*.tab.* $(FE_DIR)/lex.yy.c $(BE_DIR)/*.tab.* $(BE_DIR)/lex.be.c