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
void gencode_FunctionCall(AST_NODE* functionCallNode);

void gencode_prologue(char *name);
void gencode_epilogue(char *name);
void gencode_read(AST_NODE* functionCallNode);
void gencode_fread(AST_NODE* functionCallNode);
void gencode_write(AST_NODE* functionCallNode);



int gencode(AST_NODE* node){
    FP = fopen("output.s", "w");
	
    fprintf(FP, ".data\n");
    fprintf(FP, "_endline: .ascii \"\\n\"\n");
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
    if (variable->nodeType == EXPR_NODE) {					// INT_TYPE / FLOAT_TYPE
    	gencode_ExprNode(variable);
    } else if (variable->nodeType == STMT_NODE) {			// INT_TYPE / FLOAT_TYPE / VOID_TYPE
    	gencode_FunctionCall(variable);        
    } else if (variable->nodeType == IDENTIFIER_NODE) {		// INT_TYPE / FLOAT_TYPE / INT_PTR_TYPE / FLOAT_PTR_TYPE
    	gencode_VariableRValue(variable);    
    } else if (variable->nodeType == CONST_VALUE_NODE) {	// INT_TYPE / FLOAT_TYPE / CONST_STRING_TYPE;
    	gencode_ConstValueNode(variable);    
    }
    return variable->dataType;
}

void gencode_ExprNode(AST_NODE* exprNode){
	
}

void gencode_VariableRValue(AST_NODE* idNode){
	
}

void gencode_ConstValueNode(AST_NODE* constValueNode){
	
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
                case ARRAY_ID:      // too compilcated for me QAQ
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

                    } else {

                    }
					frame_offset += 8;
                    break;
                case WITH_INIT_ID:
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
						fprintf(FP, ".data\n");
						fprintf(FP, "_CONSTANT_%d: .word %d\n", constid, variable->child->semantic_value.const1->const_u.intval);
						fprintf(FP, ".align 3\n");
						fprintf(FP, ".text\n");
						fprintf(FP, "lw t0, _CONSTANT_%d\n", constid++);
						fprintf(FP, "sw t0,%d(sp)\n", frame_offset);
                    } else {
						fprintf(FP, ".data\n");
						fprintf(FP, "_CONSTANT_%d: .float %f\n", constid, variable->child->semantic_value.const1->const_u.fval);
						fprintf(FP, ".align 3\n");
						fprintf(FP, ".text\n");
						fprintf(FP, "la t5, _CONSTANT_%d\n", constid++);
						fprintf(FP, "flw ft0,0(t5)\n");
						fprintf(FP, "fsw ft0,%d(sp)\n", frame_offset);
						/*
						.data
						_CONSTANT_0: .float 1.20
						.align 3
						.text
						la t5,_CONSTANT_0
						flw ft0,0(t5)
						fsw ft0,0(sp)
						*/
                    }
					frame_offset += 8;
                    break;
                case ARRAY_ID:      // too compilcated for me QAQ
                    //symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                    
                    break;
                default:
                    fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
            }
        }
        
        
        
        
        
        
        
        
        /*
        switch (variable->semantic_value.identifierSemanticValue.kind) {
            case NORMAL_ID:
                symbol->attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
                symbol->attribute->attr.typeDescriptor->properties.dataType = type;
                break;
            case ARRAY_ID:
                symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                break;
            case WITH_INIT_ID:
                symbol->attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
                symbol->attribute->attr.typeDescriptor->properties.dataType = type;
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
        }
        */
        
        
        if (ignoreFirstDimensionOfArray) {
            
        }
        
    }
}

void gencode_declareType(AST_NODE* typeNode) {
    
}

void gencode_declareFunction(AST_NODE* declarationNode){
    AST_NODE* returnType = declarationNode->child;
    AST_NODE* functionName = returnType->rightSibling;
    AST_NODE* functionParam = functionName->rightSibling;
    AST_NODE* block = functionParam->rightSibling;
    
    frame_offset = 0;
    char* name = functionName->semantic_value.identifierSemanticValue.identifierName;
    
    
    if(!strcmp(functionName->semantic_value.identifierSemanticValue.identifierName,"MAIN")){
        fprintf(FP, ".text\n");
        fprintf(FP, "\n\n_start_MAIN:\n\n");
    }else{
        fprintf(FP, ".text\n");
        fprintf(FP, "\n\n_%s:\n\n",name);
    }
    gencode_prologue(name);
    gencode_openScope();
    
	gencode_BlockNode(block);
	
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
                //checkWhileStmt(stmtNode);
                break;
            case FOR_STMT:
                //checkForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                //checkAssignmentStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                gencode_FunctionCall(stmtNode);
                break;
            case IF_STMT:
                //checkIfStmt(stmtNode);
                break;
            case RETURN_STMT:
                //checkReturnStmt(stmtNode);
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

void gencode_FunctionCall(AST_NODE* functionCallNode){
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    //  special case 1 and 2
    if(strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "write") == 0){
        gencode_write(functionCallNode);
        return;
    } else if (strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "read") == 0){
        gencode_read(functionCallNode);
        return;
    } else if (strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "fread") == 0){
        gencode_fread(functionCallNode);
        return;
    } else {
        // call function
        return;
    }
    /*
    // get function signature
    SymbolTableEntry* function = retrieveSymbol(functionName->semantic_value.identifierSemanticValue.identifierName,
                                /*onlyInCurrentScope/ false);
    if (function == NULL) {
        printErrorMsgSpecial(functionCallNode, functionName->semantic_value.identifierSemanticValue.identifierName,
                                SYMBOL_UNDECLARED);
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    else if (function->attribute->attributeKind != FUNCTION_SIGNATURE) {
        printErrorMsgSpecial(functionCallNode, functionName->semantic_value.identifierSemanticValue.identifierName,
                                NOT_FUNCTION_NAME);
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    else {
        checkParameterPassing(paramList, function->attribute->attr.functionSignature->parameterList, function->name);
        functionCallNode->dataType = function->attribute->attr.functionSignature->returnType;        // fixed
        //fprintf(stderr, "mark %d\n", function->attribute->attr.functionSignature->returnType);
    }
    */
}










void gencode_prologue(char *name){
    /*
    */
    fprintf(FP, "addi    sp,sp,-16\n");
    fprintf(FP, "sd      ra,8(sp)\n");
    fprintf(FP, "sd      fp,0(sp)\n");
    fprintf(FP, "mv      fp,sp\n");
    fprintf(FP, "la ra, _frameSize_%s\n",name);
    fprintf(FP, "lw ra, 0(ra)\n");
    fprintf(FP, "sub sp, sp, ra\n");
    fprintf(FP, "_begin_%s:\n\n", name);
}

void gencode_epilogue(char *name){
    /*
    */
    fprintf(FP, "\n_end_%s:\n", name);
    fprintf(FP, "mv      sp,fp\n");
    fprintf(FP, "ld      ra,8(sp)\n");
    fprintf(FP, "ld      fp,0(sp)\n");
    fprintf(FP, "addi    sp,sp,16\n");
    fprintf(FP, "jr ra\n"); 
    fprintf(FP, ".data\n");
    fprintf(FP, "_frameSize_%s:     .word    %d\n", name, frame_offset);
}

void gencode_fread(AST_NODE* functionCallNode){
    fprintf(FP, "call _read_float\n");  // data in fa0
}

void gencode_read(AST_NODE* functionCallNode){
    fprintf(FP, "call _read_int\n");    // data in a0
}

void gencode_write(AST_NODE* functionCallNode){
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;   // actually not used
    AST_NODE* param = paramList->child;
    
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(param->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
        	
	// Case 1 Const String
    if(param->dataType == CONST_STRING_TYPE){
		if( !strcmp(param->semantic_value.const1->const_u.sc, "\"\\n\"") ){
			fprintf(FP, "la t0, _endline\n");
			fprintf(FP, "mv a0,t0\n");
			fprintf(FP, "jal _write_str\n");
			return;
		}
		//fprintf(stderr, "%s\n", param->semantic_value.const1->const_u.sc);
        fprintf(FP, ".data\n");
        fprintf(FP, "_CONSTANT_%d: .ascii %s\n", constid, param->semantic_value.const1->const_u.sc);
        fprintf(FP, ".align 3\n");
        fprintf(FP, ".text\n");
        fprintf(FP, "la t0, _CONSTANT_%d\n", constid++);
        fprintf(FP, "mv a0,t0\n");
        fprintf(FP, "jal _write_str\n");
        return;
    }
	
    if(symbol == NULL || symbol->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){    // we assume cannot write array
		fprintf(stderr, "write function Error QQ\n");
        exit(0);
    }
	
    switch (symbol->attribute->attr.typeDescriptor->properties.dataType) {
		// Case 2 INT
        case INT_TYPE:
            if(symbol->global == true){
                fprintf(FP, "la t5,_g_%d\n",symbol->offset);
                fprintf(FP, "lw t0,0(t5)\n");
                fprintf(FP, "mv a0,t0\n");
                fprintf(FP, "jal _write_int\n");
            } else {
                fprintf(FP, "lw t0,%d(sp)\n",symbol->offset);
                fprintf(FP, "mv a0,t0\n");
                fprintf(FP, "jal _write_int\n");
            }
            break;
		// Case 3 FLOAT
        case FLOAT_TYPE:
            if(symbol->global == true){
                fprintf(FP, "la t5,_g_%d\n",symbol->offset);
                fprintf(FP, "flw ft0,0(t5)\n");
                fprintf(FP, "fmv.s fa0,ft0\n");
                fprintf(FP, "jal _write_float\n");
            } else {
                fprintf(FP, "flw ft0,%d(sp)\n",symbol->offset);
                fprintf(FP, "fmv.s fa0,ft0\n");
                fprintf(FP, "jal _write_float\n");
            }
            
            break;
        default:
            fprintf(stderr, "gencodewrite Error %d\n",param->dataType);
            break;
    }
}
// riscv64-unknown-linux-gnu-objdump -d a.out > tmp.s

// qemu-riscv64 -g 1234 ./a.out
// riscv64-unknown-linux-gnu-gdb
// target remote localhost:1234

// t0-t6, s2-s11, ft0-ft7