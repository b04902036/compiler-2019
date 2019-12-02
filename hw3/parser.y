/* Parser template 2019 
===== Definition Section ===== */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "header.h"
int linenumber = 1;
AST_NODE *prog;

extern int yylex();
extern int g_anyErrorOccur;
extern void printGV(AST_NODE *root, char* fileName);
int yyerror (char *mesg);

static inline AST_NODE* makeSibling(AST_NODE *a, AST_NODE *b)
{ 
    while (a->rightSibling) {
        a = a->rightSibling;
    }
    if (b == NULL) {
        return a;
    }
    b = b->leftmostSibling;
    a->rightSibling = b;
    
    b->leftmostSibling = a->leftmostSibling;
    b->parent = a->parent;
    while (b->rightSibling) {
        b = b->rightSibling;
        b->leftmostSibling = a->leftmostSibling;
        b->parent = a->parent;
    }
    return b;
}

static inline AST_NODE* makeChild(AST_NODE *parent, AST_NODE *child)
{
    if (child == NULL) {
        return parent;
    }
    if (parent->child) {
        makeSibling(parent->child, child);
    } else {
        child = child->leftmostSibling;
        parent->child = child;
        while (child) {
            child->parent = parent;
            child = child->rightSibling;
        }
    }
    return parent;
}

static AST_NODE* makeFamily(AST_NODE *parent, int childrenCount, ...)
{
    va_list childrenList;
    va_start(childrenList, childrenCount);
    AST_NODE* child = va_arg(childrenList, AST_NODE*);
    makeChild(parent, child);
    AST_NODE* tmp = child;
    int index = 1;
    for (index = 1; index < childrenCount; ++index) {
        child = va_arg(childrenList, AST_NODE*);
        tmp = makeSibling(tmp, child);
    }
    va_end(childrenList);
    return parent;
}

static inline AST_NODE* makeIDNode(char *lexeme, IDENTIFIER_KIND idKind)
{
    AST_NODE* identifier = Allocate(IDENTIFIER_NODE);
    identifier->semantic_value.identifierSemanticValue.identifierName = lexeme;
    identifier->semantic_value.identifierSemanticValue.kind = idKind;
    identifier->semantic_value.identifierSemanticValue.symbolTableEntry = NULL;
    return identifier;                        
}

static inline AST_NODE* makeStmtNode(STMT_KIND stmtKind)
{
    AST_NODE* stmtNode = Allocate(STMT_NODE);
    stmtNode->semantic_value.stmtSemanticValue.kind = stmtKind;
    return stmtNode;                        
}

static inline AST_NODE* makeDeclNode(DECL_KIND declKind)
{
    AST_NODE* declNode = Allocate(DECLARATION_NODE);
    declNode->semantic_value.declSemanticValue.kind = declKind;
    return declNode;                        
}

static inline AST_NODE* makeExprNode(EXPR_KIND exprKind, int operationEnumValue)
{
    AST_NODE* exprNode = Allocate(EXPR_NODE);
    exprNode->semantic_value.exprSemanticValue.isConstEval = 0;
    exprNode->semantic_value.exprSemanticValue.kind = exprKind;
    if (exprKind == BINARY_OPERATION) {
        exprNode->semantic_value.exprSemanticValue.op.binaryOp = operationEnumValue;
    } else if (exprKind == UNARY_OPERATION) {
        exprNode->semantic_value.exprSemanticValue.op.unaryOp = operationEnumValue;
    } else {
        printf("Error in static inline AST_NODE* makeExprNode(EXPR_KIND exprKind, int operationEnumValue)\n");
    }
    return exprNode;                        
}

%}



%union{
	char *lexeme;
	CON_Type  *const1;
	AST_NODE  *node;
};

%token <lexeme>ID
%token <const1>CONST
%token VOID    
%token INT     
%token FLOAT   
%token IF      
%token ELSE    
%token WHILE   
%token FOR 
%token TYPEDEF 
%token OP_ASSIGN  
%token OP_OR   
%token OP_AND  
%token OP_NOT  
%token OP_EQ   
%token OP_NE   
%token OP_GT   
%token OP_LT   
%token OP_GE   
%token OP_LE   
%token OP_PLUS 
%token OP_MINUS        
%token OP_TIMES        
%token OP_DIVIDE       
%token MK_LB 
%token MK_RB 
%token MK_LPAREN       
%token MK_RPAREN       
%token MK_LBRACE       
%token MK_RBRACE       
%token MK_COMMA        
%token MK_SEMICOLON    
%token MK_DOT  
%token ERROR
%token RETURN

%type <node> program global_decl_list global_decl function_decl block stmt_list decl_list decl var_decl type init_id_list init_id  stmt relop_expr relop_term relop_factor expr term factor var_ref
%type <node> param_list param dim_fn expr_null id_list dim_decl cexpr mcexpr cfactor assign_expr_list assign_expr rel_op relop_expr_list nonempty_relop_expr_list
%type <node> add_op mul_op dim_list type_decl nonempty_assign_expr_list


%start program

%%

/* ==== Grammar Section ==== */

/* Productions */               /* Semantic actions */
program		: global_decl_list { $$=Allocate(PROGRAM_NODE);  makeChild($$,$1); prog=$$;}
		| { $$=Allocate(PROGRAM_NODE); prog=$$;}
		;

global_decl_list: global_decl_list global_decl 
                    {
                        $$ = makeSibling($1, $2);
                    }	
                | global_decl
                    {
                        $$ = $1;
                    }
                ; 

global_decl	: decl_list function_decl
                {
                    $$ = makeSibling(makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1), $2);
                }
            | function_decl 
                {
                    $$ = $1;
                }
            ;

function_decl	: type ID MK_LPAREN param_list MK_RPAREN MK_LBRACE block MK_RBRACE     
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* parameterList = Allocate(PARAM_LIST_NODE);
                        makeChild(parameterList, $4);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), parameterList, $7);
                    }
                | VOID ID MK_LPAREN param_list MK_RPAREN MK_LBRACE block MK_RBRACE      
                    {
                        /* done */
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* voidType = makeIDNode("void", NORMAL_ID);
                        AST_NODE* parameterList = Allocate(PARAM_LIST_NODE);
                        makeChild(parameterList, $4);
                        makeFamily($$, 4, voidType, makeIDNode($2, NORMAL_ID), parameterList, $7);
                    }
                | type ID MK_LPAREN  MK_RPAREN MK_LBRACE block MK_RBRACE 
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* emptyParameterList = Allocate(PARAM_LIST_NODE);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), emptyParameterList, $6);
                    }
                | VOID ID MK_LPAREN  MK_RPAREN MK_LBRACE block MK_RBRACE 
                    {
                        /* done */
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* voidType = makeIDNode("void", NORMAL_ID);
                        AST_NODE* emptyParameterList = Allocate(PARAM_LIST_NODE);
                        makeFamily($$, 4, voidType, makeIDNode($2, NORMAL_ID), emptyParameterList, $6);
                    } 
                ;

param_list	: param_list MK_COMMA  param 
                {
                    $$ = makeSibling($1, $3);
                }
            | param	
                {
                    /* done */
                    $$ = $1;
                }
            ;

param		: type ID 
                {
                    $$ = makeDeclNode(FUNCTION_PARAMETER_DECL);
                    makeFamily($$, 2, $1, makeIDNode($2, NORMAL_ID));
                }
            | type ID dim_fn 
                {
                    /* done */
                    $$ = makeDeclNode(FUNCTION_PARAMETER_DECL);
                    AST_NODE *arrayDecl = makeIDNode($2, ARRAY_ID);
                    arrayDecl = makeChild(arrayDecl, $3);
                    makeFamily($$, 2, $1, arrayDecl);
                }
            ;
dim_fn		: MK_LB expr_null MK_RB 
                {
                    $$ = $2;
                }
            | dim_fn MK_LB expr MK_RB
                {
                    $$ = makeSibling($1, $3);
                }
		;

expr_null	:expr 
                {
                    /* done */
                    $$ = $1;
                }
            |
                {
                    $$ = Allocate(NUL_NODE); 
                }
            ;

block           : decl_list stmt_list 
                    {
                        /* done */
                        $$ = Allocate(BLOCK_NODE);
                        AST_NODE* varDecl = makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1);
                        AST_NODE* stmtList = makeChild(Allocate(STMT_LIST_NODE), $2);
                        makeFamily($$, 2, varDecl, stmtList);
                    }
                | stmt_list  
                    {
                        $$ = Allocate(BLOCK_NODE);
                        makeChild($$, makeChild(Allocate(STMT_LIST_NODE), $1));
                    }
                | decl_list 
                    {
                        $$ = Allocate(BLOCK_NODE);
                        makeChild($$, makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1));
                    }
                |   {
                        /* done */
                        $$ = Allocate(BLOCK_NODE);
                    }
                ;
 
decl_list	: decl_list decl 
                {
                    /* done */
                    $$ = makeSibling($1, $2);
                }
            | decl 
                {
                    /* done */
                    $$ = $1;
                }
            ;

decl		: type_decl 
                {
                    $$ = $1;
                }
            | var_decl 
                {
                    $$ = $1;
                }
            ;

type_decl 	: TYPEDEF type id_list MK_SEMICOLON  
                {
                    /* done */
                    $$ = makeDeclNode(TYPE_DECL);
                    makeFamily($$, 2, $2, $3);
                }
            | TYPEDEF VOID id_list MK_SEMICOLON 
                {
                    /* done */
                    $$ = makeDeclNode(TYPE_DECL);
                    makeFamily($$, 2, makeIDNode("void", NORMAL_ID), $3);
                }
            ;

var_decl	: type init_id_list MK_SEMICOLON 
                {
                    /* done */
                    $$ = makeDeclNode(VARIABLE_DECL);
                    makeFamily($$, 2, $1, $2);
                }
            | ID id_list MK_SEMICOLON
                {
                    /* done */
                    $$ = makeDeclNode(VARIABLE_DECL);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $2);
                }
            ;

type		: INT 
                {
                    $$ = makeIDNode("int", NORMAL_ID);  
                }
            | FLOAT 
                {
                    $$ = makeIDNode("float", NORMAL_ID);
                }
            ;

id_list		: ID 
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | id_list MK_COMMA ID 
                {
                    /* done */
                    $$ = makeSibling($1, makeIDNode($3, NORMAL_ID));
                }
            | id_list MK_COMMA ID dim_decl
                {
                    /* done */
                    AST_NODE* arrayID = makeIDNode($3, ARRAY_ID);
                    $$ = makeSibling($1, arrayID);
                    makeChild(arrayID, $4);
                }
            | ID dim_decl
                {
                    /* done */
                    $$ = makeIDNode($1, ARRAY_ID);
                    makeChild($$, $2);
                }
		;
dim_decl	: MK_LB cexpr MK_RB 
                {
                    /* done */
                    $$ = $2;
                } 
            | dim_decl MK_LB cexpr MK_RB
                {
                    /* done */
                    $$ = makeSibling($1, $3);
                }
            ;
cexpr		: cexpr OP_PLUS mcexpr 
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                    makeFamily($$, 2, $1, $3);
                } /* This is for array declarations */ 
            | cexpr OP_MINUS mcexpr
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_SUB);
                    makeFamily($$, 2, $1, $3);
                } 
            | mcexpr 
                {
                    /* done */
                    $$ = $1;
                }
            ;  
mcexpr		: mcexpr OP_TIMES cfactor 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_MUL);
                    makeFamily($$, 2, $1, $3);
                }
            | mcexpr OP_DIVIDE cfactor 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_DIV);
                    makeFamily($$, 2, $1, $3);
                }
            | cfactor 
                {
                    /* done */
                    $$ = $1;
                }
            ;
        
cfactor:	CONST 
                {
                    /* done */
                    $$ = Allocate(CONST_VALUE_NODE);
                    $$->semantic_value.const1 = $1;
                }
            | MK_LPAREN cexpr MK_RPAREN 
                {
                    /* done */
                    $$ = $2;
                }
            ;

init_id_list	: init_id 
                    {
                        /* done */
                        $$ = $1;
                    }
                | init_id_list MK_COMMA init_id 
                    {
                        /* done */
                        $$ = makeSibling($1, $3);
                    }
                ;

init_id		: ID 
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | ID dim_decl 
                {
                    /* done */
                    $$ = makeIDNode($1, ARRAY_ID);
                    makeChild($$, $2);
                }
            | ID OP_ASSIGN relop_expr 
                {
                    /* done */
                    $$ = makeIDNode($1, WITH_INIT_ID);
                    makeChild($$, $3);
                }
            ;

stmt_list	: stmt_list stmt 
                {
                    /* done */
                    $$ = makeSibling($1, $2);
                }
            | stmt
                {
                    /* done */
                    $$ = $1;
                }
            ;



stmt		: MK_LBRACE block MK_RBRACE 
                {
                    /* done */
                    $$ = $2;
                }
            | WHILE MK_LPAREN relop_expr MK_RPAREN stmt
                {
                    /* done */
                    $$ = makeStmtNode(WHILE_STMT);
                    makeFamily($$, 2, $3, $5);
                }
            | FOR MK_LPAREN assign_expr_list MK_SEMICOLON relop_expr_list MK_SEMICOLON assign_expr_list MK_RPAREN stmt
                {
                    /* done */
                    $$ = makeStmtNode(FOR_STMT);
                    makeFamily($$, 4, $3, $5, $7, $9);
                }
            | var_ref OP_ASSIGN relop_expr MK_SEMICOLON
                {
                    /* done */
                    $$ = makeStmtNode(ASSIGN_STMT);
                    makeFamily($$, 2, $1, $3);
                }
            | IF MK_LPAREN assign_expr MK_RPAREN stmt
                {
                    /* done */
                    $$ = makeStmtNode(IF_STMT);
                    makeFamily($$, 2, $3, $5);
                }
            | IF MK_LPAREN assign_expr MK_RPAREN stmt ELSE stmt
                {
                    /* done */
                    $$ = makeStmtNode(IF_STMT);
                    makeFamily($$, 3, $3, $5, $7);
                }
            | ID MK_LPAREN relop_expr_list MK_RPAREN MK_SEMICOLON
                {
                    /* done */
                    $$ = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                }
            | MK_SEMICOLON 
                {
                    $$ = Allocate(NUL_NODE);
                }
            | RETURN MK_SEMICOLON  
                {
                    /* done */
                    $$ = makeStmtNode(RETURN_STMT);
                }
            | RETURN relop_expr MK_SEMICOLON
                {
                    /* done */
                    $$ = makeStmtNode(RETURN_STMT);
                    makeChild($$, $2);
                }
            ;

assign_expr_list : nonempty_assign_expr_list 
                     {
                        /* done */
                        $$ = Allocate(NONEMPTY_ASSIGN_EXPR_LIST_NODE);
                        makeChild($$, $1);
                     }
                 |  
                     {
                         $$ = Allocate(NUL_NODE); 
                     }
                 ;

nonempty_assign_expr_list        : nonempty_assign_expr_list MK_COMMA assign_expr 
                                    {
                                        /* done */
                                        $$ = makeSibling($1, $3);
                                    }
                                 | assign_expr
                                    {
                                        /* done */
                                        $$ = $1;
                                    }
                                 ;


assign_expr     : ID OP_ASSIGN relop_expr 
                    {
                        /* done */
                        $$ = makeStmtNode(ASSIGN_STMT);
                        makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                    }
                | relop_expr
                    {
                        /* done */
                        $$ = $1;
                    }
		;

relop_expr	: relop_term 
                {
                    $$ = $1;
                }
            | relop_expr OP_OR relop_term
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_OR);
                    makeFamily($$, 2, $1, $3);
                }
            ;

relop_term	: relop_factor 
                {
                    /* done */
                    $$ = $1;
                }
            | relop_term OP_AND relop_factor
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_AND);
                    makeFamily($$, 2, $1, $3);
                }
            ;

relop_factor	: expr
                    {
                        /* done */
                        $$ = $1;
                    }
                | expr rel_op expr 
                    {
                        /* done */
                        $$ = makeFamily($2, 2, $1, $3);
                    }
                ;

rel_op		: OP_EQ
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_EQ);
                }
            | OP_GE 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_GE);
                }
            | OP_LE 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_LE);
                }
            | OP_NE 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_NE);
                }
            | OP_GT 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_GT);
                }
            | OP_LT 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_LT);
                }
            ;


relop_expr_list	: nonempty_relop_expr_list 
                    {
                        /* done */
                        $$ = Allocate(NONEMPTY_RELOP_EXPR_LIST_NODE);
                        makeChild($$, $1);
                    }
                | 
                    {
                        $$ = Allocate(NUL_NODE);
                    }
                ;

nonempty_relop_expr_list	: nonempty_relop_expr_list MK_COMMA relop_expr
                                {
                                    /* done */
                                    $$ = makeSibling($1, $3);
                                }
                            | relop_expr 
                                {
                                    /* done */
                                    $$ = $1;
                                }
                            ;

expr		: expr add_op term 
                {
                    /* done */
                    $$ = $2;
                    makeFamily($$, 2, $1, $3);
                }
            | term 
                {
                    /* done */
                    $$ = $1;
                }
            ;

add_op		: OP_PLUS
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                }
            | OP_MINUS 
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_SUB);
                }
            ;

term		: term mul_op factor
                {
                    /* done */
                    $$ = $2;
                    makeFamily($$, 2, $1, $3);
                }
            | factor
                {
                    /* done */
                    $$ = $1;
                }
            ;

mul_op		: OP_TIMES
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_MUL);
                }
            | OP_DIVIDE 
                {
                    /* done */
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_DIV);
                }
            ;

factor		: MK_LPAREN relop_expr MK_RPAREN
                {
                    /* done */
                    $$ = $2;
                }
            | OP_MINUS MK_LPAREN relop_expr MK_RPAREN
                {
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    makeChild($$, $3);
                }
            | OP_NOT MK_LPAREN relop_expr MK_RPAREN
                {   
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    makeChild($$, $3);
                }
            | CONST 
                {
                    $$ = Allocate(CONST_VALUE_NODE);
                    $$->semantic_value.const1=$1;
                }
            | OP_MINUS CONST
                {
                    /* done */
                    AST_NODE* constant = Allocate(CONST_VALUE_NODE);
                    constant->semantic_value.const1 = $2;
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    makeChild($$, constant);
                }
            | OP_NOT CONST
                {
                    /* done */
                    AST_NODE* constant = Allocate(CONST_VALUE_NODE);
                    constant->semantic_value.const1 = $2;
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    makeChild($$, constant);
                }
            | ID MK_LPAREN relop_expr_list MK_RPAREN 
                {
                    /* done */
                    $$ = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                }
            | OP_MINUS ID MK_LPAREN relop_expr_list MK_RPAREN
                {
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    AST_NODE* stmtFunctionCall = makeStmtNode(FUNCTION_CALL_STMT);
                    makeChild($$, stmtFunctionCall);
                    makeFamily(stmtFunctionCall, 2, makeIDNode($2, NORMAL_ID), $4);
                }
            | OP_NOT ID MK_LPAREN relop_expr_list MK_RPAREN
                {
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    AST_NODE* stmtFunctionCall = makeStmtNode(FUNCTION_CALL_STMT);
                    makeChild($$, stmtFunctionCall);
                    makeFamily(stmtFunctionCall, 2, makeIDNode($2, NORMAL_ID), $4);
                }
            | var_ref 
                {
                    /* done */
                    $$ = $1;
                }
            | OP_MINUS var_ref
                {
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    makeChild($$, $2);
                }
            | OP_NOT var_ref 
                {
                    /* done */
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    makeChild($$, $2);
                }
            ;

var_ref		: ID 
                {
                    /* done */
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | ID dim_list 
                {
                    /* done */
                    $$ = makeIDNode($1, ARRAY_ID);
                    makeChild($$, $2);

                }
            ;


dim_list	: dim_list MK_LB expr MK_RB 
                {
                    /* done */
                    $$ = makeSibling($1, $3);
                }
            | MK_LB expr MK_RB
                {
                    /* done */
                    $$ = $2;
                }
		;


%%

#include "lex.yy.c"
int main (argc, argv)
int argc;
char *argv[];
  {
     yyin = fopen(argv[1],"r");
     printf("start parsing...\n");
     yyparse();
	 printf("%s\n", "Parsing completed. No errors found.");
	 printGV(prog, NULL);
  } /* main */


int yyerror (mesg)
char *mesg;
  {
  printf("%s\t%d\t%s\t%s\n", "Error found in Line ", linenumber, "next token: ", yytext );
  exit(1);
  }
 
