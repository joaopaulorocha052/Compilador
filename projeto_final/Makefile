SRC = ./src
OPT = ./get_opt
INC = ./include
BUILD = ./build
GEN = ./gen


all: programa


programa: lexer.o bison.o
	gcc -I$(SRC) -I$(INC) -o $(BUILD)/comp $(GEN)/lex.yy.c $(GEN)/sintatico.tab.c $(SRC)/symbol_table.c $(SRC)/main.c -lfl


lexer.o: $(SRC)/lex.l
	flex -o $(GEN)/lex.yy.c $(SRC)/lex.l

bison.o: $(SRC)/sintatico.y
	bison -d -o $(GEN)/sintatico.tab.c $(SRC)/sintatico.y

clean:
	rm -f $(GEN)/*
	rm -f $(BUILD)/*
	
clean-obj:
	rm -f $(BIN)/*.o