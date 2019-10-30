typedef struct symtab{
	char lexeme[32];
	struct symtab *front;
	struct symtab *back;
	int line;
	int counter;
} symtab;

typedef struct printTable {
    char *name;
    int hash_key;
    struct printTable *head;
    struct printTable *tail;
} PrintTable;

symtab * lookup(char *name);
void insert(char *name);
