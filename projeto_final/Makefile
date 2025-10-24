BIN = ./bin
SRC = ./src
OPT = ./get_opt


all: programa


programa: lexer.o main.o options.o
	gcc -I$(SRC) -I$(OPT) -o $(BIN)/programa $(BIN)/options.o $(BIN)/lexer.o $(BIN)/main.o -lfl



lexer.o: $(SRC)/lex.l
	flex -o $(SRC)/lex.yy.c $(SRC)/lex.l
	gcc -I$(SRC) -c $(SRC)/lex.yy.c -o $(BIN)/lexer.o

main.o: $(SRC)/main.c
	gcc -I$(SRC) -c $(SRC)/main.c -o $(BIN)/main.o

options.o: $(OPT)/options.c
	gcc -c $(OPT)/options.c -o $(BIN)/options.o

clean:
	rm -f $(BIN)/*
	rm -f $(SRC)/lex.yy.c

clean-obj:
	rm -f $(BIN)/*.o