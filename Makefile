SRC = ./src
OPT = ./get_opt
INC = ./include
BUILD = ./build
GEN = ./gen


all: programa


debug: lexer.o bison.o
	gcc -DDEBUG_BUILD -g -Wall -I$(SRC) -I$(INC) -o $(BUILD)/debug_build $(GEN)/*.c $(SRC)/*.c -lfl

programa: lexer.o bison.o
	gcc -I$(SRC) -I$(INC) -o $(BUILD)/pinecc $(GEN)/*.c $(SRC)/*.c -lfl


programa-old: lexer.o bison.o
	gcc -I$(SRC) -I$(INC) -o $(BUILD)/comp1 $(GEN)/lex.yy.c $(GEN)/sintatico.tab.c $(SRC)/symbol_table.c $(SRC)/code_gen.c $(SRC)/main.c $(SRC)/utils.c -lfl


lexer.o: $(SRC)/lex.l
	mkdir -p gen
	mkdir -p build
	flex -o $(GEN)/lex.yy.c $(SRC)/lex.l

bison.o: $(SRC)/sintatico.y
	bison -d -o $(GEN)/sintatico.tab.c $(SRC)/sintatico.y

clean:
	rm -f $(GEN)/*
	rm -f $(BUILD)/*
	
clean-obj:
	rm -f $(BIN)/*.o
