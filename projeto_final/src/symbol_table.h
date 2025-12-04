#ifndef __SYMBOL_TABLE__
#define __SYMBOL_TABLE__

#include <stdbool.h>
#define HASH_TABLE_SIZE 256

typedef struct HashTable HashTable;
typedef struct HashItem HashItem;
typedef struct LineList LineList;

typedef enum {VAR, FUNC} ExpKind;
typedef enum {INT_EXP, VOID_EXP} ExpType;

struct LineList{
    int line_num;
    LineList* next_line;
};

struct HashItem{
    char* value;
    char* scope;
    ExpType type;
    ExpKind kind;
    LineList* lines;
    HashItem* next_item;
};

struct HashTable{
    int size;
    HashItem** items;
};

static int hash(char* key);
static void deallocate_line_list(LineList* list);
static void deallocate_item(HashItem* item);
static void deallocate_table(HashTable* table);
static LineList* create_line_list(int num);
void insert_line(HashItem* item, int num);
static HashItem* create_item(char* value, char* scope, ExpKind kind, ExpType type);
HashTable* create_table();
void insert_item(HashTable* table, char* value, char* scope, int line_num, ExpKind kind, ExpType type);
HashItem* search_item(HashTable* table, char* value, char* scope);
void print_table(HashTable* table);
const char* kind_to_string(ExpKind kind);
const char* type_to_string(ExpType type);



#endif