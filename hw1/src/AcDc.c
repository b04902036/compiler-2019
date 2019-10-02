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
			/*
			while(1){
				Token token = scanner(source);
				if(token.type==EOFsymbol)	exit(0);
				printf("%s\n",token.tok);
			}
			*/
			program = parser(source);	// 1
			fclose(source);
			symtab = build(program);	// 2
			check(&program, &symtab);	// 3
			gencode(program, target, &symtab);	// 4
			
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

	while( isdigit(c) ) {		//讀到下一個非數字為止
		token.tok[i++] = c;
		c = fgetc(source);
	}

	if( c != '.' ){				//case 1：非數字不為 '.'，所以是 int
		ungetc(c, source);
		token.tok[i] = '\0';
		token.type = IntValue;
		return token;
	}

	token.tok[i++] = '.';

	c = fgetc(source);
	if( !isdigit(c) ){			//case 2：非數字為 '.'，所以是 float
		ungetc(c, source);
		printf("Expect a digit : %c\n", c);
		exit(1);
	}

	while( isdigit(c) ){		//讀到下一個非數字為止
		token.tok[i++] = c;
		c = fgetc(source);
	}

	ungetc(c, source);
	token.tok[i] = '\0';
	token.type = FloatValue;
	return token;
}

Token scanner( FILE *source )
{
	char c;
	Token token;

	while( !feof(source) ){	//還有字元
		c = fgetc(source);	//讀一字元

		while( isspace(c) ) c = fgetc(source);	//保證獨到非空白字元

		if( isdigit(c) )						//處理數字的 token
			return getNumericToken(source, c);

		token.tok[0] = c;						//處理文字的 token
		token.tok[1] = '\0';
		if( islower(c) ){						//case 1：英文字母開頭
			char d = fgetc(source);
			int ok = (!isalpha(d))?1:0;
			ungetc(d, source);
			
			if( c == 'f' && ok ){
				token.type = FloatDeclaration;
			}else if( c == 'i' && ok ){
				token.type = IntegerDeclaration;
			}else if( c == 'p' && ok ){
				token.type = PrintOp;
			}else{
				token.type = Alphabet;
				c = fgetc(source);
				
				int id = 1;
				while( isalpha(c) ){
					token.tok[id++] = c;
					c = fgetc(source);
				}
				token.tok[id] = '\0';
				ungetc(c, source);
			}
			//printf("%s\n",token.tok);
			return token;
		}

		switch(c){								//case 2：operator、EOF、exception
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
			token2 = scanner(source);						// f token2 或是 i token2
			if (strcmp(token2.tok, "f") == 0 ||				// 要保證 token2 不能是 'f i p'
					strcmp(token2.tok, "i") == 0 ||
					strcmp(token2.tok, "p") == 0) {
				printf("Syntax Error: %s cannot be used as id\n", token2.tok);
				exit(1);
			}
			return makeDeclarationNode( token, token2 );
		default:											// 基本不會發生
			printf("Syntax Error: Expect Declaration %s\n", token.tok);
			exit(1);
	}
}

Declarations *parseDeclarations( FILE *source )
{
	Token token = scanner(source);
	Declaration decl;
	Declarations *decls;
	switch(token.type){
		case FloatDeclaration:							//Case 1 : float 或是 int 宣告
		case IntegerDeclaration:
			decl = parseDeclaration(source, token);
			decls = parseDeclarations(source);
			return makeDeclarationTree( decl, decls );
		case PrintOp:									//Case 2 : 讀到 Stms，return 然後呼叫 parseStatements
		case Alphabet:
			for(int i = strlen(token.tok)-1;i>=0;i--)
				ungetc(token.tok[i], source);
			return NULL;
		case EOFsymbol:									//讀到 EOF 等價於 Case 2
			return NULL;
		default:										//Case 3 : 讀到其他東西，表示 Syntax Error
			printf("Syntax Error: Expect declarations %s\n", token.tok);
			exit(1);
	}
}

/*
Origin:	
	RHS = value ET
	ET = + value ET | - value ET | λ
	
-----------------------------------
New:
	RHS = term ET
	ET = + term ET | - term ET | λ
	
	Term = value TT
	TT = * value TT | / value TT | λ
	
	I use the same structure for Term and Expression, but their order of parsing are different.
*/

Expression *parseValue( FILE *source )
{
	Token token = scanner(source);
	Expression *value = (Expression *)malloc( sizeof(Expression) );
	value->leftOperand = value->rightOperand = NULL;

	switch(token.type){
		case Alphabet:								//id
			(value->v).type = Identifier;
			strcpy((value->v).val.id,token.tok);
			break;
		case IntValue:								//int value
			(value->v).type = IntConst;
			(value->v).val.ivalue = atoi(token.tok);
			break;
		case FloatValue:							//float value
			(value->v).type = FloatConst;
			(value->v).val.fvalue = atof(token.tok);
			break;
		default:									//Syntax error
			printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
			exit(1);
	}

	return value;
}



Expression *parseTermTail( FILE *source, Expression *lvalue )
{
	Token token = scanner(source);
	Expression *expr;

	switch(token.type){
		case MulOp:
			expr = (Expression *)malloc( sizeof(Expression) );
			(expr->v).type = MulNode;
			(expr->v).val.op = Mul;
			expr->leftOperand = lvalue;
			expr->rightOperand = parseValue(source);
			return parseTermTail(source, expr);
		case DivOp:
			expr = (Expression *)malloc( sizeof(Expression) );
			(expr->v).type = DivNode;
			(expr->v).val.op = Div;
			expr->leftOperand = lvalue;
			expr->rightOperand = parseValue(source);
			return parseTermTail(source, expr);
		case Alphabet:						// 下一個 staments
		case PrintOp:
		case PlusOp:
		case MinusOp:
			for(int i = strlen(token.tok)-1;i>=0;i--)
				ungetc(token.tok[i], source);
			return lvalue;
		case EOFsymbol:						// 讀完整個檔案
			return lvalue;
		default:							//Syntax error
			printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
			exit(1);
	}
}

Expression *parseTerm( FILE *source)
{
	Expression *value = parseValue(source);
	return parseTermTail(source, value);
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
			expr->rightOperand = parseTerm(source);
			return parseExpressionTail(source, expr);
		case MinusOp:
			expr = (Expression *)malloc( sizeof(Expression) );
			(expr->v).type = MinusNode;
			(expr->v).val.op = Minus;
			expr->leftOperand = lvalue;
			expr->rightOperand = parseTerm(source);
			return parseExpressionTail(source, expr);
		case Alphabet:						// 下一個 staments
		case PrintOp:
			for(int i = strlen(token.tok)-1;i>=0;i--)
				ungetc(token.tok[i], source);
			return lvalue;
		case EOFsymbol:						// 讀完整個檔案
			return lvalue;
		default:							//Syntax error
			printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
			exit(1);
	}
}


Statement parseStatement( FILE *source, Token token )
{
	Token next_token;
	Expression *value, *expr;

	switch(token.type){
		case Alphabet:										// Case 1 : id = xxx
			next_token = scanner(source);
			if(next_token.type == AssignmentOp){				
				value = parseTerm(source);
				expr = parseExpressionTail(source, value);	//注意到 parseExpression 和 parseExpression 幾乎做一樣的事
				return makeAssignmentNode(token.tok, expr);		// value is not used inside of entering function
			}
			else{												
				printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
				exit(1);
			}
		case PrintOp:										// Case 2 : p id
			next_token = scanner(source);
			if(next_token.type == Alphabet){			
				return makePrintNode(next_token.tok);
			}else{												
				printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
				exit(1);
			}
			break;
		default:											// 基本不會發生
			printf("Syntax Error: Expect a statement %s\n", token.tok);
			exit(1);
	}
}

Statements *parseStatements( FILE * source )
{
	Token token = scanner(source);
	Statement stmt;
	Statements *stmts;

	switch(token.type){
		case Alphabet:								//Case 1: id = xxx
		case PrintOp:								//Case 2: p id
			stmt = parseStatement(source, token);
			stmts = parseStatements(source);
			return makeStatementTree(stmt , stmts);
		case EOFsymbol:								//Case 3: 讀完整個程式了
			return NULL;
		default:									//Case 4: Syntax Error
			printf("Syntax Error: Expect statements %s\n", token.tok);
			exit(1);
	}
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
	//printf("%s\n%s\n",declare_type.tok, identifier.tok);
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
	strcpy(tree_node.name,identifier.tok);

	return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
	Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
	new_tree->first = decl;
	new_tree->rest = decls;

	return new_tree;
}

void ConstantFolding( Expression *expr )
{
	Expression *left = expr->leftOperand;
	Expression *right = expr->rightOperand;
	
	if(left == NULL || right == NULL)	return;
	
	ConstantFolding(left);
	ConstantFolding(right);
	
	if(left->v.type != IntConst && left->v.type != FloatConst)		return;
	if(right->v.type != IntConst && right->v.type != FloatConst)	return;
	
	int leftint,rightint;
	leftint = (left->v.type == IntConst) ? 1 : 0;
	rightint = (right->v.type == IntConst) ? 1 : 0;
	
	
	switch(expr->v.type){
		case PlusNode:
			if(leftint&&rightint)		expr->v.type = IntConst, 	expr->v.val.ivalue = left->v.val.ivalue + right->v.val.ivalue;
			else if(leftint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.ivalue + (float)right->v.val.fvalue;
			else if(rightint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue + (float)right->v.val.ivalue;
			else						expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue + (float)right->v.val.fvalue;
			break;
		case MinusNode:
			if(leftint&&rightint)		expr->v.type = IntConst, 	expr->v.val.ivalue = left->v.val.ivalue - right->v.val.ivalue;
			else if(leftint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.ivalue - (float)right->v.val.fvalue;
			else if(rightint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue - (float)right->v.val.ivalue;
			else						expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue - (float)right->v.val.fvalue;
			break;
		case MulNode:
			if(leftint&&rightint)		expr->v.type = IntConst, 	expr->v.val.ivalue = left->v.val.ivalue * right->v.val.ivalue;
			else if(leftint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.ivalue * (float)right->v.val.fvalue;
			else if(rightint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue * (float)right->v.val.ivalue;
			else						expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue * (float)right->v.val.fvalue;
			break;
		case DivNode:
			if(leftint&&rightint)		expr->v.type = IntConst, 	expr->v.val.ivalue = left->v.val.ivalue / right->v.val.ivalue;
			else if(leftint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.ivalue / (float)right->v.val.fvalue;
			else if(rightint)			expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue / (float)right->v.val.ivalue;
			else						expr->v.type = FloatConst, 	expr->v.val.fvalue = (float)left->v.val.fvalue / (float)right->v.val.fvalue;
			break;
			
		default:
			break;
	}
	//printf("%d %d %d %d %d\n",lefti, righti, lefti * righti, lefti / righti, expr->v.val.ivalue);
	free(expr->leftOperand);
	free(expr->rightOperand);
	expr->leftOperand = NULL;
	expr->rightOperand = NULL;
	return;
}

Statement makeAssignmentNode( char *id, Expression *expr_tail )	// in this version, v is not used
{
	Statement stmt;
	stmt.type = Assignment;
	
	
	AssignmentStatement assign;
	strcpy(assign.id, id);
	//print_expr(expr_tail);
	//printf("\n");
	ConstantFolding(expr_tail);
	//print_expr(expr_tail);
	//printf("\n");
	assign.expr = expr_tail;
	
	stmt.stmt.assign = assign;

	return stmt;
}

Statement makePrintNode( char *id )
{
	Statement stmt;
	stmt.type = Print;
	strcpy(stmt.stmt.variable, id);

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
	for(i = 0 ; i < 23; i++)
		for(int j = 0; j < 65; j++)
			table->map[i][j] = '\0';
	table->id = 0;
}

int mapping(SymbolTable *table, char *str){
	for(int i = 0; i < (table->id) ;i++)
		if(strcmp(str,table->map[i])==0)
			return i;
		
	strcpy(table->map[(table->id)],str);
	table->id = table->id + 1;
	return table->id - 1;
}

void add_table( SymbolTable *table, char *c, DataType t )
{
	// int index = (int)(c - 'a');
	int index = mapping(table, c);
	
	if(table->table[index] != Notype)
		printf("Error : id %s has been declared\n", c);//error
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
			printf("convert to float %s \n",old->v.val.id);
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

DataType lookup_table( SymbolTable *table, char *c )
{
	//int id = c[0]-'a';
	int id = mapping(table, c);
	
	if( table->table[id] != Int && table->table[id] != Float)
		printf("Error : identifier %s is not declared\n", c);//error
	return table->table[id];
}

void checkexpression( Expression * expr, SymbolTable * table )
{
	char c[65];
	if(expr->leftOperand == NULL && expr->rightOperand == NULL){
		switch(expr->v.type){
			case Identifier:
				stpcpy(c, expr->v.val.id);
				printf("identifier : %s\n",c);
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
	else{		//PlusNode, MinusNode, MulNode, DivNode
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
		printf("assignment : %s \n",assign.id);
		checkexpression(assign.expr, table);
		stmt->stmt.assign.type = lookup_table(table, assign.id);
		if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
			printf("error : can't convert float to integer\n");
		} else {
			convertType(assign.expr, stmt->stmt.assign.type);
		}
	}
	else if (stmt->type == Print){
		printf("print : %s \n",stmt->stmt.variable);
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
void fprint_op( FILE *target, ValueType op)
{
	switch(op){
		case MinusNode:
			fprintf(target,"-\n");
			break;
		case PlusNode:
			fprintf(target,"+\n");
			break;
		case MulNode:
			fprintf(target, "*\n");
			break;
		case DivNode:
			fprintf(target, "/\n");
			break;
		default:
			fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
			break;
	}
}

void fprint_expr( FILE *target, Expression *expr, SymbolTable *table)
{

	if(expr->leftOperand == NULL){
		switch( (expr->v).type ){
			case Identifier:
				fprintf(target,"l%c\n",mapping(table, (expr->v).val.id)+'a');
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
		fprint_expr(target, expr->leftOperand, table);
		if(expr->rightOperand == NULL){
			fprintf(target,"5k\n");
		}
		else{
			//	fprint_right_expr(expr->rightOperand);
			fprint_expr(target, expr->rightOperand, table);
			fprint_op(target, (expr->v).type);
		}
	}
}

void gencode(Program prog, FILE * target, SymbolTable *table)
{
	Statements *stmts = prog.statements;
	Statement stmt;

	while(stmts != NULL){
		stmt = stmts->first;
		switch(stmt.type){
			case Print:
				fprintf(target,"l%c\n",mapping(table,stmt.stmt.variable)+'a');
				fprintf(target,"p\n");
				break;
			case Assignment:
				fprint_expr(target, stmt.stmt.assign.expr, table);
				/*
				   if(stmt.stmt.assign.type == Int){
				   fprintf(target,"0 k\n");
				   }
				   else if(stmt.stmt.assign.type == Float){
				   fprintf(target,"5 k\n");
				   }*/
				fprintf(target,"s%c\n",mapping(table,stmt.stmt.assign.id)+'a');
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
				printf("%s ", (expr->v).val.id);
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
		printf("%s ",decl.name);
		decls = decls->rest;
	}

	stmts = program.statements;

	while(stmts != NULL){
		stmt = stmts->first;
		if(stmt.type == Print){
			printf("p %s ", stmt.stmt.variable);
		}

		if(stmt.type == Assignment){
			printf("%s = ", stmt.stmt.assign.id);
			print_expr(stmt.stmt.assign.expr);
		}
		stmts = stmts->rest;
	}

}