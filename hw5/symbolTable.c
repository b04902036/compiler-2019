#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(const char* str) {
	int idx=0;
	while (*str){
		idx <<= 1;
		idx += (int)(*str);
		++str;
	}
	return (idx & (HASH_TABLE_SIZE-1));
}

SymbolTable symbolTable;

SymbolTableEntry* newSymbolTableEntry()
{
    SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    symbolTableEntry->nextInHashChain = NULL;
    symbolTableEntry->prevInHashChain = NULL;
    symbolTableEntry->nextInSameLevel = NULL;
    symbolTableEntry->sameNameInOuterLevel = NULL;
    symbolTableEntry->attribute = NULL;
    symbolTableEntry->name = NULL;
    symbolTableEntry->nestingLevel = symbolTable.currentLevel;
    return symbolTableEntry;
}

// probably no need
void removeFromHashTrain(int hashIndex, SymbolTableEntry* entry)
{
    ;
}

/**
 * add a symbol into particular hash table entry
 * if the entry is not empty, 
 *   add the symbol to start of the entry
 */
void addIntoHashChain(SymbolTableEntry** hashTable, 
        int hashIndex, SymbolTableEntry* symbol) {
    if (hashTable[hashIndex] == NULL) {
        hashTable[hashIndex] = symbol;
    }
    else {
        hashTable[hashIndex]->prevInHashChain = symbol;
        symbol->nextInHashChain = hashTable[hashIndex];
        hashTable[hashIndex] = symbol;
    }
}

void initializeSymbolTable() {
    symbolTable.currentLevel = 0;

    symbolTable.hashTable = NULL;
    symbolTable.totalHashTableCount = 0;
    symbolTable.allocatedHashTableCount = 0;

    symbolTable.totalScope = NULL;
    //symbolTable.totalScopeCount = 0;
    symbolTable.allocatedTotalScopeCount = 0;
	
	symbolTable.open_order = NULL;
	symbolTable.very_total = 0;
}



// wtf is this
void symbolTableEnd()
{
    ;
}

/**
 * given a symbol name, get the entry in hash table
 * NEED TO CHECK IF THE NAME IS THE SAME in case for COLLISION
 * return NULL if not found
 */
SymbolTableEntry* retrieveSymbol(const char* symbolName, bool onlyInCurrentScope) {
    int idx = HASH(symbolName);
    int maxHashTableId = symbolTable.totalHashTableCount - 1;
    SymbolTableEntry* ret;
    for (int hashTableId = maxHashTableId; hashTableId > -1; --hashTableId) {
        ret = symbolTable.hashTable[hashTableId][idx];
        while (ret != NULL) {
            if (!strcmp(ret->name, symbolName)) {
                return ret;
            }
            ret = ret->nextInHashChain;
        }

        if (onlyInCurrentScope) {
            return NULL;
        }
    }
    
    // if reach here, no declaration is found
    return NULL;
}

/**
 * add symbol into symbolTable
 */
void addSymbol(SymbolTableEntry* symbol) {
    int idx = HASH(symbol->name);
    int hashTableId = symbol->nestingLevel - 1;
    addIntoHashChain(symbolTable.hashTable[hashTableId], idx, symbol);
}

//remove the symbol from the current scope
// probably no need
void removeSymbol(char* symbolName)
{
    ;
}

// wtf is this
int declaredLocally(char* symbolName)
{
    ;
}

/**
 * create a new hash table
 * 1. check if current array of hash table is big enough
 * 2. allocate for new hash table
 * 3. increase hash table count
 */
void openScope() {
    // these variables are just to shorten the name...
    int current = symbolTable.totalHashTableCount;
    int already = symbolTable.allocatedHashTableCount;

    // 1.
    if (current >= already) {
        symbolTable.hashTable = (SymbolTableEntry***) realloc(symbolTable.hashTable, sizeof(SymbolTableEntry**) * (current + 1));
		symbolTable.open_order = (int*) realloc( symbolTable.open_order, sizeof(int) * (current + 1));
        symbolTable.allocatedHashTableCount = current + 1;
    }

    // 2.
    symbolTable.hashTable[current] = (SymbolTableEntry**) calloc(HASH_TABLE_SIZE, sizeof(SymbolTableEntry*));
	symbolTable.open_order[current] = symbolTable.very_total;

    // 3.
    ++symbolTable.totalHashTableCount;
    ++symbolTable.currentLevel;
	++symbolTable.very_total;
}

/**
 * store the top hash table to total scope for other "pass" to use
 * 1. check if totalScope is big enough
 * 2. move hash table to totalScope
 * 3. subtract number of hashtable and increase number of totalScope
 */
void closeScope() {
    // these variables are just to shorten the name...
    int currentHash = symbolTable.totalHashTableCount - 1;
    //int current = symbolTable.totalScopeCount;
    int current = symbolTable.open_order[currentHash];	// magic !
    int already = symbolTable.allocatedTotalScopeCount;
    
    // 1.
    if (current >= already) {
        // allocate three at once
        symbolTable.totalScope = (SymbolTableEntry***) realloc(symbolTable.totalScope, sizeof(SymbolTableEntry**) * (current+1));
        symbolTable.allocatedTotalScopeCount = current+1;
    }
    
    // 2.
    symbolTable.totalScope[current] = symbolTable.hashTable[currentHash];

    // 3.
    --symbolTable.currentLevel;
    --symbolTable.totalHashTableCount;
    //++symbolTable.totalScopeCount;
}



// gencode retrive in the second time.
void gencode_init_SymbolTable() {
    symbolTable.currentLevel = 0;

    symbolTable.hashTable = NULL;
    symbolTable.totalHashTableCount = 0;
    symbolTable.allocatedHashTableCount = 0;
	
	free(symbolTable.open_order);
    symbolTable.scope_now = 0;
}

void gencode_openScope() {
    // these variables are just to shorten the name...
    int current = symbolTable.totalHashTableCount;
    int already = symbolTable.allocatedHashTableCount;

    // 1.
    if (current >= already) {
        symbolTable.hashTable = (SymbolTableEntry***) realloc(symbolTable.hashTable, sizeof(SymbolTableEntry**) * (current + 1));
		symbolTable.allocatedHashTableCount = current + 1;
    }

    // 2.
    symbolTable.hashTable[current] = symbolTable.totalScope[symbolTable.scope_now];

    // 3.
    ++symbolTable.totalHashTableCount;
    ++symbolTable.currentLevel;	
    ++symbolTable.scope_now;
}

void gencode_closeScope() {	
    ++symbolTable.currentLevel;	
    --symbolTable.scope_now;
}

SymbolTableEntry* gencode_retrieveSymbol(const char* symbolName, bool onlyInCurrentScope) {
    int idx = HASH(symbolName);
    int maxHashTableId = symbolTable.totalHashTableCount - 1;
    SymbolTableEntry* ret;
    for (int hashTableId = maxHashTableId; hashTableId > -1; --hashTableId) {
        ret = symbolTable.hashTable[hashTableId][idx];
        while (ret != NULL) {
            if (!strcmp(ret->name, symbolName)) {
                return ret;
            }
            ret = ret->nextInHashChain;
        }

        if (onlyInCurrentScope) {
            return NULL;
        }
    }
    
    // if reach here, no declaration is found
    return NULL;
}