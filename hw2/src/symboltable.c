#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	512


symtab * hash_table[TABLE_SIZE];
PrintTable *print_table;
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));	
	
	hash_key=HASH(name);
	ptr=hash_table[hash_key];

	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	
	}
	
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;

    // insert word into print_table
    PrintTable *tmp_table = print_table;
    do {
        // stop if less then next
        if (strcmp(name, tmp_table->name) < 0) {
            PrintTable *new_block = (PrintTable *) malloc(sizeof(PrintTable));
            new_block->name = (char *) malloc(strlen(name) + 1);
            strcpy(new_block->name, name);
            new_block->hash_key = hash_key;
            new_block->head = tmp_table->head;
            new_block->tail = tmp_table;
            tmp_table->head->tail = new_block;
            tmp_table->head = new_block;
            return;
        }
    } while ((tmp_table->tail != NULL) && (tmp_table = tmp_table->tail));
    
    // insert to the tail
    PrintTable *new_block = (PrintTable *) malloc(sizeof(PrintTable));
    new_block->name = (char *) malloc(strlen(name) + 1);
    strcpy(new_block->name, name);
    new_block->hash_key = hash_key;
    new_block->head = tmp_table;
    new_block->tail = NULL;
    tmp_table->tail = new_block;
}

void printSym(symtab* ptr) 
{
	    printf("%s\t", ptr->lexeme);
	    printf("%d\n", ptr->counter);
}

void printSymTab()
{
    int i;
    printf("Frequency of identifiers:\n");
    symtab *ptr;
    do {
        print_table = print_table->tail;
        ptr = hash_table[print_table->hash_key];
        while (strcmp(ptr->lexeme, print_table->name) != 0) 
            ptr = ptr->front;
        printSym(ptr);
    } while (print_table->tail != NULL);
}
