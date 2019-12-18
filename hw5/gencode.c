// int / float instruction, I use it a little bit randomly QQ !!.

// TODO :   
//			1.Expression
//			2.if/else
//			3.while
//          4.array

#include "header.h"
#include "symbolTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE * FP;
int constid = 0;
int global_offset = 0;
int frame_offset = 0;
bool global_variable_declaration = true;

int gencode(AST_NODE* node);
void gencode_Program(AST_NODE* node);
void gencode_DeclarationNode(AST_NODE* declarationNode);
void gencode_declareIdList(AST_NODE* declarationNode, bool ignoreFirstDimensionOfArray, Parameter* paramList);
void gencode_declareType(AST_NODE* typeNode);
void gencode_declareFunction(AST_NODE* declarationNode);

DATA_TYPE gencode_RelopExpr(AST_NODE* variable);
void gencode_ExprNode(AST_NODE* exprNode);
void gencode_VariableRValue(AST_NODE* idNode); 
void gencode_ConstValueNode(AST_NODE* constValueNode);

void gencode_BlockNode(AST_NODE* blockNode);
void gencode_StmtNode(AST_NODE* stmtNode);
void gencode_FunctionCall(AST_NODE* functionCallNode, bool assignR2L);
void gencode_AssignmentStmt(AST_NODE* assignmentNode);

void gencode_ForStmt(AST_NODE* forNode);
void gencode_WhileStmt(AST_NODE* whileNode);
void gencode_IfStmt(AST_NODE* ifNode);
void gencode_ReturnStmt(AST_NODE* returnNode);

void gencode_prologue(char *name);
void gencode_epilogue(char *name);
void store_register();
void restore_register();
void gencode_read(AST_NODE* idNode);
void gencode_fread(AST_NODE* idNode);
void gencode_write(AST_NODE* functionCallNode);

// I assume that no (a1+(a2+(a3+(a4+(a5+(...))))))
// Only need in expression in function, store at those ast nodes.
// t0-t6, ft0-ft7: temp for int and float
// s2-s11:	how 2 use
// a0:		return value
// a1-a7:	parameter passing
bool reg_arr[15];
const char regmap[15][4] = {"t0","t1","t2","t3","t4","t5","t6","ft0","ft1","ft2","ft3","ft4","ft5","ft6","ft7"};
void init_register();
int get_register(bool getint);
int free_register(int id);
void check_t6();
void check_register_notused();



int gencode(AST_NODE* node){
    FP = fopen("output.s", "w");
    
    fprintf(FP, ".data\n");
    fprintf(FP, "_endline: .ascii \"\\n\\000\"\n");
    fprintf(FP, ".align 3\n");
    
    gencode_Program(node);
    fclose(FP);
    return 0;
}

void gencode_Program(AST_NODE* programNode){
    AST_NODE* start = programNode->child;
    AST_NODE* tmp;
    
    //fprintf(stderr, "+++\n");
    gencode_openScope();

    for (start = programNode->child; start != NULL; start = start->rightSibling) {
        switch (start->nodeType) {
            case DECLARATION_NODE:                  // function declaration
                //fprintf(FP, ".text\n");
                global_variable_declaration = false;
                gencode_DeclarationNode(start);
                break;
            case VARIABLE_DECL_LIST_NODE:           // decl_list 
                //fprintf(FP, ".data\n");
                global_variable_declaration = true;
                for (tmp = start->child; tmp != NULL; tmp = tmp->rightSibling) {
                    gencode_DeclarationNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such type of node\n");
                exit(255);
        }
    }
    gencode_closeScope();
}

DATA_TYPE gencode_RelopExpr(AST_NODE* variable) {
    if (variable->nodeType == EXPR_NODE) {                  // INT_TYPE / FLOAT_TYPE
        gencode_ExprNode(variable);
    } else if (variable->nodeType == STMT_NODE) {           // INT_TYPE / FLOAT_TYPE / VOID_TYPE
        gencode_FunctionCall(variable, true);        
    } else if (variable->nodeType == IDENTIFIER_NODE) {     // INT_TYPE / FLOAT_TYPE / INT_PTR_TYPE / FLOAT_PTR_TYPE
        gencode_VariableRValue(variable);    
    } else if (variable->nodeType == CONST_VALUE_NODE) {    // INT_TYPE / FLOAT_TYPE / CONST_STRING_TYPE;
        gencode_ConstValueNode(variable);    
    }
    return variable->dataType;
}

void gencode_ExprNode(AST_NODE* exprNode){
    // Finally
}

void gencode_VariableRValue(AST_NODE* idNode){
	SymbolTableEntry *symbol = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, false);
	if (symbol==NULL){
        fprintf(stderr, "RValue Error.\n");
        exit(255);
	}
	
    if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
		int reg_place;
		if(symbol->global == true) {	// global
            if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
				reg_place = get_register(true);
				idNode->reg_place = reg_place;
				fprintf(FP, "lw      %s,_g_%d\n", regmap[reg_place], symbol->offset);
            } else {	
				check_t6();
				reg_place = get_register(false);
				idNode->reg_place = reg_place;
				fprintf(FP, "la      t6,_g_%d\n", symbol->offset);
				fprintf(FP, "flw     %s,0(t6)\n", regmap[reg_place]);
            }
		} else {	// local
            if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
				reg_place = get_register(true);
				idNode->reg_place = reg_place;
				fprintf(FP, "lw      %s,%d(sp)\n", regmap[reg_place], symbol->offset);
			} else {
				reg_place = get_register(false);
				idNode->reg_place = reg_place;
				fprintf(FP, "flw     %s,%d(sp)\n", regmap[reg_place], symbol->offset);
            }
		}
    }
    else {	// Array Element
		// Array Element
	}
}

void gencode_ConstValueNode(AST_NODE* constValueNode){
	int reg_place;
    switch (constValueNode->semantic_value.const1->const_type) {
        case INTEGERC:
            //fprintf(stderr, "Gencode Int.\n");
			reg_place = get_register(true);
			constValueNode->reg_place = reg_place;
            fprintf(FP, "li      %s,%d\n", regmap[reg_place], constValueNode->semantic_value.const1->const_u.intval);
            return;
        case FLOATC:
            //fprintf(stderr, "Gencode Float.\n");
			check_t6();
			reg_place = get_register(false);
			constValueNode->reg_place = reg_place;
            fprintf(FP, ".data\n");
            fprintf(FP, "_CONSTANT_%d: .float %f\n", constid, constValueNode->semantic_value.const1->const_u.fval);
            fprintf(FP, ".align 3\n");
            fprintf(FP, ".text\n");
            fprintf(FP, "la      t6, _CONSTANT_%d\n", constid++);
            fprintf(FP, "flw     %s,0(t6)\n",regmap[reg_place]);
            return;
        default:	// cannot be string, string is used only in write, I deal with it in other place.
            fprintf(stderr, "Gencode Const Value Error.\n");
            exit(255);
    }
}

void gencode_DeclarationNode(AST_NODE* declarationNode){
    switch (declarationNode->semantic_value.declSemanticValue.kind) {
        case VARIABLE_DECL:                                 // int a, b, c[];
            gencode_declareIdList(declarationNode, false, NULL);
            break;
        case TYPE_DECL:                                     // typedef int aaa
            gencode_declareType(declarationNode);
            break;
        case FUNCTION_DECL:                                 // int f(int a, int b){}
            gencode_declareFunction(declarationNode);
            break;
        default:
            fprintf(stderr, "no such DECL kind\n");
            exit(255);
    }
}
void gencode_declareIdList(AST_NODE* declarationNode, bool ignoreFirstDimensionOfArray, Parameter* paramList){
    AST_NODE* dataType = declarationNode->child;
    for (AST_NODE* variable = dataType->rightSibling; variable != NULL; variable = variable->rightSibling) {
        SymbolTableEntry* symbol = gencode_retrieveSymbol(variable->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/true);
        if (symbol == NULL) {
            fprintf(stderr, "no such symbol %s in symbol table\n",variable->semantic_value.identifierSemanticValue.identifierName);
            exit(255);
        }
        
        if (global_variable_declaration == true) { // global declaration
            symbol->global = true;
            symbol->offset = global_offset;
            fprintf(FP, ".data\n");
            
            switch (variable->semantic_value.identifierSemanticValue.kind) {
                case NORMAL_ID:
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
                        fprintf(FP, "_g_%d:  .word 0\n",global_offset++);
                    } else {
                        fprintf(FP, "_g_%d:  .float 0\n",global_offset++);
                    }
                    break;
                case WITH_INIT_ID:  // !!! We only handle constant initializations "int a = 3;"
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
                        fprintf(FP, "_g_%d:  .word %d\n",global_offset++, variable->child->semantic_value.const1->const_u.intval);
                    } else {
                        fprintf(FP, "_g_%d:  .float %f\n",global_offset++, variable->child->semantic_value.const1->const_u.fval);
                    }
                    break;
                case ARRAY_ID:      // global ARRAY Declare
                    //symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                    
                    break;
                default:
                    fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
            }
        }else{  // local declaration
            symbol->global = false;
            symbol->offset = frame_offset;
            
            switch (variable->semantic_value.identifierSemanticValue.kind) {
                case NORMAL_ID:
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
						// nothing to do
						frame_offset += 8;
                    } else {
						// nothing to do
						frame_offset += 8;
                    }
                    break;
                case WITH_INIT_ID:  // !!! We only handle constant initializations "int b = 4;"
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
                        fprintf(FP, "li      t0,%d\n", variable->child->semantic_value.const1->const_u.intval);
                        fprintf(FP, "sw      t0,%d(sp)\n", frame_offset);
						frame_offset += 8;
                    } else {
                        fprintf(FP, ".data\n");
                        fprintf(FP, "_CONSTANT_%d: .float %f\n", constid, variable->child->semantic_value.const1->const_u.fval);
                        fprintf(FP, ".align 3\n");
                        fprintf(FP, ".text\n");
                        fprintf(FP, "la      t5, _CONSTANT_%d\n", constid++);
                        fprintf(FP, "flw     ft0,0(t5)\n");
                        fprintf(FP, "fsw     ft0,%d(sp)\n", frame_offset);
						frame_offset += 8;
                    }
                    break;
                case ARRAY_ID:      // local ARRAY Declare
                    //symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                    
                    break;
                default:
                    fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
            }
        }
        
        
        if (ignoreFirstDimensionOfArray) {
            
        }
        
    }
}

void gencode_declareType(AST_NODE* typeNode) {
    // nothing to do !
}

void gencode_declareFunction(AST_NODE* declarationNode){
    AST_NODE* returnType = declarationNode->child;
    AST_NODE* functionName = returnType->rightSibling;
    AST_NODE* functionParam = functionName->rightSibling;
    AST_NODE* block = functionParam->rightSibling;
    
    frame_offset = 184;
    char* name = functionName->semantic_value.identifierSemanticValue.identifierName;
    
    
    if(!strcmp(functionName->semantic_value.identifierSemanticValue.identifierName,"MAIN")){
        fprintf(FP, "\n\n\n.text\n");
        fprintf(FP, "_start_MAIN:\n");
    }else{
        fprintf(FP, "\n\n\n.text\n");
        fprintf(FP, "_start_%s:\n",name);
    }
    gencode_prologue(name);
    
	gencode_openScope();
	
    init_register();	// only init register when declare function, handle e = (a+(b+(c+d))), register store temp value
	gencode_BlockNode(block);
    check_register_notused();
	
	gencode_closeScope();
	
    gencode_epilogue(name);
}

void gencode_BlockNode(AST_NODE* blockNode){
    AST_NODE *nodeptr, *tmp;
    for(nodeptr = blockNode->child; nodeptr != NULL; nodeptr = nodeptr -> rightSibling){// 2 loops at most actually
        switch(nodeptr->nodeType) {
            case VARIABLE_DECL_LIST_NODE:                           // decl_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {        // maybe we don't do this
                    gencode_DeclarationNode(tmp);
                }
                break;
            case STMT_LIST_NODE:                                    // stmt_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {
                    gencode_StmtNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] void processBlockNode(AST_NODE* blockNode).\n");
                blockNode->dataType = ERROR_TYPE;
                exit(255);
        }
    }
}

void gencode_StmtNode(AST_NODE* stmtNode){
    if(stmtNode->nodeType == BLOCK_NODE){
        //fprintf(stderr, "---\n");
        gencode_openScope();
        gencode_BlockNode(stmtNode);
        gencode_closeScope();
    }else if(stmtNode->nodeType == NUL_NODE){
        return;
    }else if(stmtNode->nodeType == STMT_NODE){
        STMT_KIND kind = stmtNode->semantic_value.stmtSemanticValue.kind;
        switch(kind){
            case WHILE_STMT:
                gencode_WhileStmt(stmtNode);
                break;
            case FOR_STMT:
                gencode_ForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                gencode_AssignmentStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                gencode_FunctionCall(stmtNode, false);
                break;
            case IF_STMT:
                gencode_IfStmt(stmtNode);
                break;
            case RETURN_STMT:
                gencode_ReturnStmt(stmtNode);
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] void processStmtNode(AST_NODE* stmtNode).\n");
                stmtNode->dataType = ERROR_TYPE;
                exit(255);
        }
    }else{
        fprintf(stderr, "[PARSER ERROR] void processStmtNode(AST_NODE* stmtNode).\n");
        stmtNode->dataType = ERROR_TYPE;
        exit(255);
    }
}

void gencode_FunctionCall(AST_NODE* functionCallNode, bool assignR2L){
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    // Special case : write ( I put read into gencode_assignment )
    if(strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "write") == 0) {
        gencode_write(functionCallNode);
        return;
    }
    
	
    // Assume ParameterLess,  		
	if(paramList->nodeType == NUL_NODE) {
		fprintf(FP, "jal _start_%s\n", functionName->semantic_value.identifierSemanticValue.identifierName);
		return;
	}else{
		
		
	}
	
	// Return Value originally put on int -> a0, float -> fa0
	// we should put into register if call from relop!!! 
	if(assignR2L){
		int reg_place;
		if(functionCallNode->dataType == INT_TYPE) {	// int
			reg_place = get_register(true);
			functionCallNode->reg_place = reg_place;
			fprintf(FP, "mv      %s,a0\n", regmap[reg_place]);
		} else {	// float
			reg_place = get_register(false);
			functionCallNode->reg_place = reg_place;
			fprintf(FP, "fmv.s   %s,fa0\n", regmap[reg_place]);
		}
	}
	return;
}

void gencode_AssignmentStmt(AST_NODE* assignmentNode){
    AST_NODE *var_ref = assignmentNode->child;
    AST_NODE *relop_expr = var_ref->rightSibling;

    // Special case : Read a = read();      b = fread();, I assume that only 2 case, not deep.
    if (relop_expr->child && strcmp(relop_expr->child->semantic_value.identifierSemanticValue.identifierName, "read") == 0){
        if (var_ref->dataType != INT_TYPE) {
            fprintf(stderr, "read error.\n");
            exit(255);
        }
        gencode_read(var_ref);
        return;
    } else if (relop_expr->child && strcmp(relop_expr->child->semantic_value.identifierSemanticValue.identifierName, "fread") == 0){
        if (var_ref->dataType != FLOAT_TYPE) {
            fprintf(stderr, "fread error.\n");
            exit(255);
        }
        gencode_fread(var_ref);
        return;
    }
    
	// Right Right Right Handside	-- 	relop_expr
	// After this line, we have data in regmap[relop_expr->reg_place]
    gencode_RelopExpr(relop_expr);
	
	
	
	// Left Left Left Handside	-- 	var_ref
	SymbolTableEntry *symbol = retrieveSymbol(var_ref->semantic_value.identifierSemanticValue.identifierName, /*onlyInCurrentScope*/false);
	if (symbol==NULL){
        fprintf(stderr, "Assign Left Handside Error.\n");
        exit(255);
	}
	
	if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
		if(symbol->global == true) {	// global
			check_t6();
            if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
				fprintf(FP, "la      t6,_g_%d\n", symbol->offset);
				fprintf(FP, "sw      %s,0(t6)\n", regmap[relop_expr->reg_place]);
            } else {	
				fprintf(FP, "la      t6,_g_%d\n", symbol->offset);
				fprintf(FP, "fsw     %s,0(t6)\n", regmap[relop_expr->reg_place]);
            }
		} else {	// local
            if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
                fprintf(FP, "sw      %s,%d(sp)\n", regmap[relop_expr->reg_place], symbol->offset);
			} else {
                fprintf(FP, "fsw     %s,%d(sp)\n", regmap[relop_expr->reg_place], symbol->offset);
            }
		}
		free_register(relop_expr->reg_place);
    }
    else {	// Array Element	
		// Array Element
	}
}



void gencode_ForStmt(AST_NODE* forNode){
    
}

void gencode_WhileStmt(AST_NODE* whileNode){
    
}

void gencode_IfStmt(AST_NODE* ifNode){
    
}

void gencode_ReturnStmt(AST_NODE* returnNode){
	// put value on a0 or fa0
    
}








void gencode_prologue(char *name){
    fprintf(FP, "addi    sp,sp,-16\n");
    fprintf(FP, "sd      ra,8(sp)\n");
    fprintf(FP, "sd      fp,0(sp)\n");
    fprintf(FP, "mv      fp,sp\n");
    fprintf(FP, "la      ra, _frameSize_%s\n",name);
    fprintf(FP, "lw      ra, 0(ra)\n");
    fprintf(FP, "sub     sp, sp, ra\n");
	//store_register();
    fprintf(FP, "_begin_%s:\n\n", name);
}

void gencode_epilogue(char *name){
    fprintf(FP, "\n_end_%s:\n", name);
	//restore_register();
    fprintf(FP, "mv      sp,fp\n");
    fprintf(FP, "ld      ra,8(sp)\n");
    fprintf(FP, "ld      fp,0(sp)\n");
    fprintf(FP, "addi    sp,sp,16\n");
    fprintf(FP, "jr ra\n"); 
    fprintf(FP, ".data\n");
    fprintf(FP, "_frameSize_%s:     .word    %d\n", name, frame_offset);
}

void store_register(){
    fprintf(FP, "sd t0,8(sp)\n");
    fprintf(FP, "sd t1,16(sp)\n");
    fprintf(FP, "sd t2,24(sp)\n");
    fprintf(FP, "sd t3,32(sp)\n");
    fprintf(FP, "sd t4,40(sp)\n");
    fprintf(FP, "sd t5,48(sp)\n");
    fprintf(FP, "sd t6,56(sp)\n");
    fprintf(FP, "sd s2,64(sp)\n");
    fprintf(FP, "sd s3,72(sp)\n");
	fprintf(FP, "sd s4,80(sp)\n");
	fprintf(FP, "sd s5,88(sp)\n");
	fprintf(FP, "sd s6,96(sp)\n");
	fprintf(FP, "sd s7,104(sp)\n");
	fprintf(FP, "sd s8,112(sp)\n");
	fprintf(FP, "sd s9,120(sp)\n");
	fprintf(FP, "sd s10,128(sp)\n");
	fprintf(FP, "sd s11,136(sp)\n");
	fprintf(FP, "sd fp,144(sp)\n");
	fprintf(FP, "fsw ft0,152(sp)\n");
	fprintf(FP, "fsw ft1,156(sp)\n");
	fprintf(FP, "fsw ft2,160(sp)\n");
	fprintf(FP, "fsw ft3,164(sp)\n");
	fprintf(FP, "fsw ft4,168(sp)\n");
	fprintf(FP, "fsw ft5,172(sp)\n");
	fprintf(FP, "fsw ft6,176(sp)\n");
	fprintf(FP, "fsw ft7,180(sp)\n");
}
void restore_register(){
    fprintf(FP, "ld t0,8(sp)\n");
    fprintf(FP, "ld t1,16(sp)\n");
    fprintf(FP, "ld t2,24(sp)\n");
    fprintf(FP, "ld t3,32(sp)\n");
    fprintf(FP, "ld t4,40(sp)\n");
    fprintf(FP, "ld t5,48(sp)\n");
    fprintf(FP, "ld t6,56(sp)\n");
    fprintf(FP, "ld s2,64(sp)\n");
    fprintf(FP, "ld s3,72(sp)\n");
	fprintf(FP, "ld s4,80(sp)\n");
	fprintf(FP, "ld s5,88(sp)\n");
	fprintf(FP, "ld s6,96(sp)\n");
	fprintf(FP, "ld s7,104(sp)\n");
	fprintf(FP, "ld s8,112(sp)\n");
	fprintf(FP, "ld s9,120(sp)\n");
	fprintf(FP, "ld s10,128(sp)\n");
	fprintf(FP, "ld s11,136(sp)\n");
	fprintf(FP, "ld fp,144(sp)\n");
	fprintf(FP, "flw ft0,152(sp)\n");
	fprintf(FP, "flw ft1,156(sp)\n");
	fprintf(FP, "flw ft2,160(sp)\n");
	fprintf(FP, "flw ft3,164(sp)\n");
	fprintf(FP, "flw ft4,168(sp)\n");
	fprintf(FP, "flw ft5,172(sp)\n");
	fprintf(FP, "flw ft6,176(sp)\n");
	fprintf(FP, "flw ft7,180(sp)\n");
}


void gencode_fread(AST_NODE* idNode){
    fprintf(FP, "call    _read_float\n");  // data in fa0
    fprintf(FP, "fmv.s   ft0,fa0\n");
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
    
	if(symbol==NULL){
		fprintf(stderr, "fread error\n");
		exit(255);
	}
    
	if(symbol->global == true){
        fprintf(FP, "la      t5,_g_%d\n",symbol->offset);
        fprintf(FP, "fsw     ft0,0(t5)\n");
    } else {
        fprintf(FP, "fsw     ft0,%d(sp)\n",symbol->offset);
    }
}

void gencode_read(AST_NODE* idNode){
    fprintf(FP, "call    _read_int\n");    // data in a0
    fprintf(FP, "mv      t0,a0\n");
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
    
	if(symbol==NULL){
		fprintf(stderr, "fread error\n");
		exit(255);
	}
    
	if(symbol->global == true){
        fprintf(FP, "la      t5,_g_%d\n",symbol->offset);
        fprintf(FP, "sw      t0,0(t5)\n");
    } else {
        fprintf(FP, "sw      t0,%d(sp)\n",symbol->offset);
    }
}

void gencode_write(AST_NODE* functionCallNode){
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;   // actually not used
    AST_NODE* param = paramList->child;
    
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(param->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
            
    // Case 1 Const String
    if(param->dataType == CONST_STRING_TYPE){
        if( !strcmp(param->semantic_value.const1->const_u.sc, "\"\\n\"") ){
            fprintf(FP, "la      t0,_endline\n");
            fprintf(FP, "mv      a0,t0\n");
            fprintf(FP, "jal     _write_str\n");
            return;
        }
        //fprintf(stderr, "%s\n", param->semantic_value.const1->const_u.sc);
        fprintf(FP, ".data\n");
		
		char name[4096];
		sprintf(name, "%s000\"", param->semantic_value.const1->const_u.sc);
		name[strlen(name)-5]='\\';
		
        fprintf(FP, "_CONSTANT_%d: .ascii %s\n", constid, name);
        fprintf(FP, ".align 3\n");
        fprintf(FP, ".text\n");
        fprintf(FP, "la      t0, _CONSTANT_%d\n", constid++);
        fprintf(FP, "mv      a0,t0\n");
        fprintf(FP, "jal     _write_str\n");
        return;
    }
    
    if(symbol == NULL || symbol->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){    // we assume cannot write array
        fprintf(stderr, "write function Error QQ\n");
        exit(255);
    }
    
    switch (symbol->attribute->attr.typeDescriptor->properties.dataType) {
        // Case 2 INT
        case INT_TYPE:
            if(symbol->global == true){
                fprintf(FP, "la      t5,_g_%d\n",symbol->offset);
                fprintf(FP, "lw      t0,0(t5)\n");
                fprintf(FP, "mv      a0,t0\n");
                fprintf(FP, "jal     _write_int\n");
            } else {
                fprintf(FP, "lw      t0,%d(sp)\n",symbol->offset);
                fprintf(FP, "mv      a0,t0\n");
                fprintf(FP, "jal     _write_int\n");
            }
            break;
        // Case 3 FLOAT
        case FLOAT_TYPE:
            if(symbol->global == true){
                fprintf(FP, "la      t5,_g_%d\n",symbol->offset);
                fprintf(FP, "flw     ft0,0(t5)\n");
                fprintf(FP, "fmv.s   fa0,ft0\n");
                fprintf(FP, "jal     _write_float\n");
            } else {
                fprintf(FP, "flw     ft0,%d(sp)\n",symbol->offset);
                fprintf(FP, "fmv.s   fa0,ft0\n");
                fprintf(FP, "jal     _write_float\n");
            }
            
            break;
        default:
            fprintf(stderr, "gencodewrite Error %d\n",param->dataType);
            break;
    }
}


void init_register(){
	memset(reg_arr,0,sizeof(reg_arr));
}

int get_register(bool getint){
	if(getint){
		for(int i=0; i<7;i++){
			if(reg_arr[i]==false){
				reg_arr[i] = true;
				return i;
			}
		}
		fprintf(stderr, "We don't have enough money to buy int register QQ. Program Crash!!!\n");
		exit(255);
	}else{
		for(int i=7; i<15;i++){
			if(reg_arr[i]==false){
				reg_arr[i] = true;
				return i;
			}
		}
		fprintf(stderr, "We don't have enough money to buy float register QQ. Program Crash!!!\n");
		exit(255);
	}
}

int free_register(int id){
	reg_arr[id] = false;
}

void check_t6(){
	if(reg_arr[6]==true){
		fprintf(stderr, "No space for -t6- to load addr of const float.\n");
		exit(255);
	}
}

void check_register_notused(){
	for(int i=0; i<15;i++){
		if(reg_arr[i]==true){
			fprintf(stderr, "Some register is blocked error\n");
			exit(255);
		}
	}
}


// riscv64-unknown-linux-gnu-objdump -d a.out > tmp.s

// qemu-riscv64 -g 1234 ./a.out
// riscv64-unknown-linux-gnu-gdb
// target remote localhost:1234

// t0-t6, s2-s11, ft0-ft7