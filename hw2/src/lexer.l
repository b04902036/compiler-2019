%option noyywrap
%{
#include <stdio.h>
#include "header.h" 
int linenumber = 0;
int Total_Of_Different_ID = 0;
symtab * lookup();
void     insertID();
void     printSymTab();
%}




letter          [A-Za-z]
digit           [0-9]
ID              {letter}({letter}|{digit}|"_")*
WS              [ \t]+

Cint            [-]?[0-9]+
First           ({Cint}"."[0-9]+?)|({Cint}?"."[0-9]+)
Second          [Ee][+-]?[0-9]+
Cfloat          {First}{Second}?    
CString         ["]((\\.)|[^"])*["]

Return      return
Typedef     typedef
If          if
Else        else
Int         int
Float       float
For         for
Void        void
While       while

OP_add      "+"
OP_sub      "-"
OP_mul      "*"
OP_div      "/"
OP_less     "<"
OP_great    ">"
OP_lesseq   "<="
OP_greateq  ">="
OP_eq       "=="
OP_noteq    "!="
OP_or     "||"
OP_and    "&&"
OP_not    "!"
OP_assign "="

DL_lparen  "("
DL_rparen  ")"
DL_lbrack  "["
DL_rbrack  "]"
DL_lbrace  "{"
DL_rbrace  "}"
DL_comma   ","
DL_semicol ";"
DL_dot     "."

newline   "\n"

comment         \/\*((\*+[^/])|([^*]))*([*]?)\*\/

error    .


%%

{Return}        
{Typedef}       
{If}            
{Else}          
{Int}           
{Float}         
{For}           
{Void}          
{While}         


{newline}       linenumber += 1;
{comment}       printf("%s\n", yytext);

{Cint}          //printf("cint %s\n",yytext);
{Cfloat}        //printf("cfloat %s\n",yytext);
{CString}       printf("cstring %s\n",yytext);

{OP_add}        {}
{OP_sub}        {}
{OP_mul}        {}
{OP_div}        {}
{OP_less}       {}
{OP_great}      {}
{OP_lesseq}     {}
{OP_greateq}    {}
{OP_eq}         {}
{OP_noteq}      {}
{OP_or}         {}
{OP_and}        {}
{OP_not}        {}
{OP_assign}     {}

{DL_lparen}     {}
{DL_rparen}     {}
{DL_lbrack}     {} 
{DL_rbrack}     {}
{DL_lbrace}     {}
{DL_rbrace}     {}
{DL_comma}      {}
{DL_semicol}    {}
{DL_dot}        {}

{WS}            {}
{ID}            { 
                    symtab *ptr = lookup(yytext);
                    if (ptr == NULL)    {Total_Of_Different_ID++;insertID(yytext);}
                    else                ptr->counter++;
                }
                
{error}         printf("Error:%s\n", yytext);return -1;



%%




int main(int argc, char **argv) // main function 
{
    if (argc > 1)   yyin = fopen(argv[1], "r");
    else            yyin = stdin;
    yylex();                    // *** lex scannar generator ***
    printSymTab();
}

