#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;
    longToChar = (VariableToReg *) malloc(sizeof(VariableToReg));
    longToChar->usedReg = 0;
    
    memset(hashTable, 0, sizeof(hashTable));

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            // constant folding
            fold(&program);
            // add type conversion
            check(&program, &symtab);
            // generate dc code
            gencode(program, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}


/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token getAlphabeticToken( FILE *source, Token token, VariableToReg *longToChar )
{
    int cnt = 0;

    // turn string into char
    token.tok[0] = hash(token.tok);
    token.tok[1] = '\0';

    return token; 
}

Token scanner( FILE *source )
{
    char c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);

        if( isdigit(c) )
            return getNumericToken(source, c);

        int idx = 1;
        token.tok[0] = c;

        c = fgetc(source);
        while (islower(c)) {
            token.tok[idx++] = c;
            c = fgetc(source);
        }
        ungetc(c, source);
        token.tok[idx] = '\0';
        
        if (islower(token.tok[0])){
            if (!strcmp(token.tok, "f"))
                token.type = FloatDeclaration;
            else if (!strcmp(token.tok, "i"))
                token.type = IntegerDeclaration;
            else if (!strcmp(token.tok, "p"))
                token.type = PrintOp;
            else
                token.type = Alphabet;
            
            if(token.type == Alphabet)
                return getAlphabeticToken(source, token, longToChar);
            else
                return token;
        }

        switch(token.tok[0]){
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
            default:
                printf("Invalid character : %c\n", c);
                exit(1);
        }
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            // care about alphabetic only
            token2 = scanner(source);
            if (strlen(token2.tok) == 1 &&
                    (token2.tok[0] == 'f' ||
                     token2.tok[0] == 'i' ||
                     token2.tok[0] == 'p')) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    // only care about f. i
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case Alphabet:
            lookupAndUngetString(token.tok[0], source, longToChar);
            return NULL;
        case PrintOp:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseValue( FILE *source )
{
    char id;
    Token token = scanner(source);
    Expression *value = (Expression *)malloc( sizeof(Expression) );
    value->leftOperand = value->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (value->v).type = Identifier;
            (value->v).val.id = token.tok[0];
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MulOp:
            switch ((lvalue->v).type) {
                case PlusNode:
                case MinusNode:
                    expr = (Expression *) malloc(sizeof(Expression));
                    (expr->v).type = MulNode;
                    (expr->v).val.op = Mul;
                    expr->leftOperand = lvalue->rightOperand;
                    expr->rightOperand = parseValue(source);
                    lvalue->rightOperand = expr;
                    return parseExpressionTail(source, lvalue);
                case MulNode:
                case DivNode:
                    expr = (Expression *) malloc(sizeof(Expression));
                    (expr->v).type = MulNode;
                    (expr->v).val.op = Mul;
                    expr->leftOperand = lvalue;
                    expr->rightOperand = parseValue(source);
                    return parseExpressionTail(source, expr);
                default:
                    printf("an operatiion neither + - * / inside a equation. should not happen\n");
                    exit(255);
            }
        case DivOp:
            switch ((lvalue->v).type) {
                case PlusNode:
                case MinusNode:
                    expr = (Expression *) malloc(sizeof(Expression));
                    (expr->v).type = DivNode;
                    (expr->v).val.op = Div;
                    expr->leftOperand = lvalue->rightOperand;
                    expr->rightOperand = parseValue(source);
                    lvalue->rightOperand = expr;
                    return parseExpressionTail(source, lvalue);
                case MulNode:
                case DivNode:
                    expr = (Expression *) malloc(sizeof(Expression));
                    (expr->v).type = DivNode;
                    (expr->v).val.op = Div;
                    expr->leftOperand = lvalue;
                    expr->rightOperand = parseValue(source);
                    return parseExpressionTail(source, expr);
                default:
                    printf("an operatiion neither + - * / inside a equation. should not happen\n");
                    exit(255);
            }
        case Alphabet:
            lookupAndUngetString(token.tok[0], source, longToChar);
            return lvalue;
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MulOp:
            expr = (Expression *) malloc(sizeof(Expression));
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case DivOp:
            expr = (Expression *) malloc(sizeof(Expression));
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case Alphabet:
            lookupAndUngetString(token.tok[0], source, longToChar);
            return NULL;
        case PrintOp:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *value, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                value = parseValue(source);
                expr = parseExpression(source, value);
                return makeAssignmentNode(token.tok[0], value, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok[0]);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{
    // only cares about assignment or print
    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    char id;
    Declaration tree_node;
    
    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }

    tree_node.name = identifier.tok[0];

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char id, Expression *v, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    assign.id = id;
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char id )
{
    Statement stmt;
    stmt.type = Print;
    stmt.stmt.variable = id;

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < 26; i++)
        table->table[i] = Notype;
}

void add_table( SymbolTable *table, char c, DataType t )
{
    int index = (int)(c - 'a');

    if(table->table[index] != Notype)
        printf("Error : id %c has been declared\n", c);//error
    table->table[index] = t;
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while(decls !=NULL){
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        return;
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %c \n",old->v.val.id);
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table( SymbolTable *table, char c )
{
    int id = c-'a';
    if( table->table[id] != Int && table->table[id] != Float)
        printf("Error : identifier %c is not declared\n", c);//error
    return table->table[id];
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char c;
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch((expr->v).type){
            case Identifier:
                c = (expr->v).val.id;
                printf("identifier : %c\n",c);
                expr->type = lookup_table(table, c);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        convertType(left, type);//left->type = type;//converto
        convertType(right, type);//right->type = type;//converto
        expr->type = type;
    }
}

void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %c \n",assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %c \n",stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target,"*\n");
            break;
        case DivNode:
            fprintf(target,"/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, Expression *expr)
{

    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                fprintf(target,"l%c\n",(expr->v).val.id);
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    }
    else{
        fprint_expr(target, expr->leftOperand);
        if(expr->rightOperand == NULL){
            fprintf(target,"5k\n");
        }
        else{
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%c\n",stmt.stmt.variable);
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                fprintf(target,"s%c\n",stmt.stmt.assign.id);
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%c ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%c ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %c ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%c = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}

// return value:
// -2 : requested expr is not a leaf
// -1 : requested expr is a leaf, and (expr->v).type is a variable
// other : requested expr is a leaf, return (expr->v).type
int constantFolding(Expression *expr) {
    if (expr->leftOperand == NULL && expr->rightOperand == NULL) {
        // this should be int, float or variable
        switch ((expr->v).type) {
            case Identifier:
                return -2;
            case IntConst:
            case FloatConst:
                return (expr->v).type;
            default:
                printf("this op should not appear here.\n");
                exit(255);
        }
    }
    else if (expr->leftOperand == NULL || expr->rightOperand == NULL) {
        printf("ConvertOp or AssignOp appear"\
                " while doing constant folding,"\
                " should not happen\n");
        exit(255);
    }
    else {
        int is_left_leaf = constantFolding(expr->leftOperand);
        int is_right_leaf = constantFolding(expr->rightOperand);
        if (is_left_leaf > -1 && is_right_leaf > -1) {
            switch ((expr->v).val.op) {
                case Plus:
                    if (is_left_leaf == IntConst && is_right_leaf == IntConst) {
                        (expr->v).type = IntConst;
                        (expr->v).val.ivalue = expr->leftOperand->v.val.ivalue +
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == IntConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.ivalue +
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == IntConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue +
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue +
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else {
                        printf("neither IntConst nor FloatConst, should not happen\n");
                        exit(255);
                    }
                    break;
                case Minus:
                    if (is_left_leaf == IntConst && is_right_leaf == IntConst) {
                        (expr->v).type = IntConst;
                        (expr->v).val.ivalue = expr->leftOperand->v.val.ivalue -
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == IntConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.ivalue -
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == IntConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue -
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue -
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else {
                        printf("neither IntConst nor FloatConst, should not happen\n");
                        exit(255);
                    }
                    break;
                case Mul:
                    if (is_left_leaf == IntConst && is_right_leaf == IntConst) {
                        (expr->v).type = IntConst;
                        (expr->v).val.ivalue = expr->leftOperand->v.val.ivalue *
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == IntConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.ivalue *
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == IntConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue *
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue *
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else {
                        printf("neither IntConst nor FloatConst, should not happen\n");
                        exit(255);
                    }
                    break;
                case Div:
                    if (is_left_leaf == IntConst && is_right_leaf == IntConst) {
                        (expr->v).type = IntConst;
                        (expr->v).val.ivalue = expr->leftOperand->v.val.ivalue /
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == IntConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.ivalue /
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == IntConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue /
                                             expr->rightOperand->v.val.ivalue;
                    }
                    else if (is_left_leaf == FloatConst && is_right_leaf == FloatConst) {
                        (expr->v).type = FloatConst;
                        (expr->v).val.fvalue = expr->leftOperand->v.val.fvalue /
                                             expr->rightOperand->v.val.fvalue;
                    }
                    else {
                        printf("neither IntConst nor FloatConst, should not happen\n");
                        exit(255);
                    }
                    break;
                default:
                    printf("neither + - * /, should not happen\n");
                    exit(255);
            }
            expr->leftOperand = NULL;
            expr->rightOperand = NULL;
            return (expr->v).type;
        }
        else 
            return -1;
    }

}

void fold(Program *program) {
    Statements *stmts = program->statements;
    while(stmts != NULL){
        if (stmts->first.type == Assignment)
            constantFolding(stmts->first.stmt.assign.expr);
        stmts = stmts->rest;
    }
}

void lookupAndUngetString(char c, FILE *f, VariableToReg *longToChar) {
    // lookup c in longToChar
    int whichName = c - 'a';
    int idx = strlen(longToChar->name[whichName]) - 1;
    while (idx > -1) {
        ungetc(longToChar->name[whichName][idx--], f);
    }
    return;
}

char hash(char input[]) {
    // get hash value
    int length = strlen(input);
    int output = 0;
    int nowReg;
    for (int i = 0; i < length; ++i)
        output = (output ^ (((int)input[i]) << 2)) % HASHTABLESIZE;
    
    // fill in hash table
    while (hashTable[output].name != NULL) {
        // check if this hash value is set already
        if (!strcmp(input, hashTable[output].name)) {
            // this hash value is already set
            return hashTable[output].id;
        }
        output = (output + 1) % HASHTABLESIZE;
    }

    // hash value not set yet
    hashTable[output].name = (char *) malloc(sizeof(char) * MAXNAME);
    nowReg = longToChar->usedReg++;
    strncpy(hashTable[output].name, input, MAXNAME);
    hashTable[output].id = 'a' + nowReg;

    // set longToChar table for inverse indexing
    strncpy(longToChar->name[nowReg], input, sizeof(longToChar->name[nowReg]));
    return 'a' + nowReg;
}

