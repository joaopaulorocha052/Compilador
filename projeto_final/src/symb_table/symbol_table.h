#ifndef __SYMBOL_TABLE__
#define __SYMBOL_TABLE__

#define HASH_TABLE_SIZE 256

typedef struct HashTable HashTable;
typedef struct HashItem HashItem;
typedef struct LineList LineList;

struct LineList{
    int line_num;
    LineList* next_line;
};

struct HashItem{
    char* value;
    char* scope;
    LineList* lines;
    HashItem* next_item;
};

struct HashTable{
    int size;
    HashItem** items;
};


#endif