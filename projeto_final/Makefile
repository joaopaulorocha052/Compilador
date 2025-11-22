BIN = ./bin
SRC = ./src
OPT = ./get_opt
INC = ./include
GEN = ./gen

INCLUDES = -I$(INC) -I$(GEN)
DEBUGS = -Wall -g

all: programa


programa:sintatico.o parse.o lexer.o main.o options.o  
	gcc -I$(SRC) -I$(OPT) $(INCLUDES) -o $(BIN)/programa $(BIN)/options.o $(BIN)/lexer.o $(BIN)/main.o $(BIN)/parse.o $(BIN)/sintatico.o -lfl

debug:sintatico.o parse.o lexer.o main.o options.o  
	gcc $(DEBUGS) -I$(SRC) -I$(OPT) $(INCLUDES) -o $(BIN)/programa $(BIN)/options.o $(BIN)/lexer.o $(BIN)/main.o $(BIN)/parse.o $(BIN)/sintatico.o -lfl

gen_comp: sintatico.o lexer.o
	gcc $(INCLUDES) -o $(BIN)/programa $(BIN)/lexer.o $(BIN)/sintatico.o -lfl

parse.o: $(SRC)/parse.c
	gcc -g $(INCLUDES) -c $(SRC)/parse.c -o $(BIN)/parse.o

sintatico.o: $(SRC)/sintatico.y
	bison --defines=$(GEN)/sintatico.tab.h -o $(GEN)/parser.tab.c $(SRC)/sintatico.y
	gcc -g -I$(INC) -c $(GEN)/parser.tab.c -o $(BIN)/sintatico.o

lexer.o: $(SRC)/lex.l
	flex -o $(GEN)/lex.yy.c $(SRC)/lex.l
	gcc -g -I$(INC) -c $(GEN)/lex.yy.c -o $(BIN)/lexer.o

main.o: $(SRC)/main.c
	gcc -g $(INCLUDES) -c $(SRC)/main.c -o $(BIN)/main.o

options.o: $(OPT)/options.c
	gcc -g -c $(OPT)/options.c -o $(BIN)/options.o

clean:
	rm -f $(BIN)/*
	rm -f $(GEN)/*

clean-obj:
	rm -f $(BIN)/*.o