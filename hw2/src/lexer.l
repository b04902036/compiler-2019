%option noyywrap 
%{
#include <stdio.h>
#include <string.h>
#include "header.h" 

extern PrintTable *print_table;

int linenumber;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
#define INT		10
#define FLOAT 		11 
#define OP_ASSIGN 	12 
#define OP_OR 		13 
#define MK_LPAREN 	14 
#define MK_RPAREN 	15 
#define ERROR 		100 

%}

letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+

OP_assign        "="
OP_and  "&&"
OP_not  "!"
OP_or   "||"
OP_add  "+"
OP_mul  "*"
OP_div  "/"
OP_minus    "-"
OP_g    ">"
OP_b    "<"
OP_ge   >=
OP_be   <=
OP_e    ==
OP_ne   !=

/* key word */
KEY_return  return
KEY_typedef typedef
KEY_if  if
KEY_else    else
KEY_int int
KEY_float   float
KEY_for for
KEY_void    void
KEY_while   while


/* Other operators appear here */ 


newline  "\n"

DL_lbracket "["
DL_rbracket "]"
DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."
error    .

/* int constant */
int_suffix ([lL][uU]?)|(((ll)|(LL))[uU]?)|([uU]([lL]?|(ll)?|(LL)?))
Int_constant {digit}+{int_suffix}?

/* floating point constant */
exponent    [eE][+-]?{digit}+
dotted_digits   ({digit}+\.)|({digit}+\.{digit}+)|(\.{digit}+)
floating_suffix [fFlL]
Float_constant (({digit}+){exponent}{floating_suffix}?)|({dotted_digits}{exponent}?{floating_suffix}?)

/* string constant */
String_constant (\")((\\.)|[^"])*(\")

/* comment */
Comment ((\/\/.*?{newline})|\/\*([^*]|\*[^/])*[*]?\*\/)


%%

{WS}            {}
{Int_constant}          {printf("int : %s\n", yytext);}
{Float_constant}       {/* return FLOAT; */}
{String_constant}       {}

{KEY_return}  
{KEY_typedef} 
{KEY_if}  
{KEY_else}    
{KEY_int}          
{KEY_float} 
{KEY_for} 
{KEY_void}    
{KEY_while}   


{ID}            { 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
		}



{OP_assign}     /* return OP_ASSIGN; */ 
{OP_and}
{OP_not}
{OP_or}         /* return OP_OR; */
{OP_add}
{OP_mul}
{OP_minus}
{OP_div}
{OP_g}
{OP_b}  
{OP_ge}   
{OP_be}   
{OP_e}    
{OP_ne}  

{DL_lbracket}
{DL_rbracket}
{DL_lparen}     /* return MK_LPAREN; */
{DL_rparen}     /* return MK_RPAREN; */
{DL_lbrace}     /* return MK_LBRACE; */
{DL_rbrace}     /* return MK_RBRACE; */
{DL_comma}      {}
{DL_semicol}    {}
{Comment}       {
                    size_t length = strlen(yytext) - 1;
                    if (yytext[length] == '\n')
                        yytext[length] = '\0';
                    printf("%s\n", yytext);}
{newline}       linenumber += 1;
{error}         printf("ERR %s\n", yytext);/* return ERROR; */


%%
int main(int argc, char **argv)
{
 argc--; ++argv;

   print_table = (PrintTable *) malloc(sizeof(PrintTable));
   print_table->name = calloc(1, 1);
   print_table->head = NULL;
   print_table->tail = NULL;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}

