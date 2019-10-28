#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE  512

symtab * hash_table[TABLE_SIZE];
extern int linenumber;
extern int Total_Of_Different_ID;

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
}

int cmp(const void *a,const void*b){
    symtab *aa = (symtab *)a;
    symtab *bb = (symtab *)b;
    return strcmp(aa->lexeme,bb->lexeme);
}
void printSymTab()
{
    int id = 0;
    symtab *data = (symtab*)malloc(Total_Of_Different_ID*sizeof(symtab));
    
    printf("Frequency of identifiers:\n");
    for (int i=0; i<TABLE_SIZE; i++){
        symtab* symptr;
        symptr = hash_table[i];
        while (symptr != NULL){
            data[id++] = *symptr;
            symptr=symptr->front;
        }
    }
    qsort(data,Total_Of_Different_ID,sizeof(data[0]),cmp);
    for(int i = 0;i<Total_Of_Different_ID;i++)
        printf("%s\t%d\n",data[i].lexeme,data[i].counter);
}
