#include "header.h"
#include "symbolTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE * FP;
int constid = 0;

int gencode(AST_NODE* node);
void gencodeProgram(AST_NODE* node);
void gencodeDeclarationNode(AST_NODE* declarationNode);
void gencodedeclareFunction(AST_NODE* declarationNode);
void gencodeBlockNode(AST_NODE* blockNode);
void gencodeStmtNode(AST_NODE* stmtNode);
void gencodeFunctionCall(AST_NODE* functionCallNode);

void gencodeprologue(char *name);
void gencodeepilogue(char *name);
void gencoderead(AST_NODE* functionCallNode);
void gencodefread(AST_NODE* functionCallNode);
void gencodewrite(AST_NODE* functionCallNode);



int gencode(AST_NODE* node){
	FP = fopen("output.s", "w");
	gencodeProgram(node);
	fclose(FP);
	return 0;
}

void gencodeProgram(AST_NODE* programNode){
    AST_NODE* start = programNode->child;
    AST_NODE* tmp;
    
    gencode_openScope();

    for (start = programNode->child; start != NULL; start = start->rightSibling) {
        switch (start->nodeType) {
            case DECLARATION_NODE:                  // function declaration
				//fprintf(FP, ".text\n");
                gencodeDeclarationNode(start);
                break;
            case VARIABLE_DECL_LIST_NODE:           // decl_list 
				//fprintf(FP, ".data\n");
                for (tmp = start->child; tmp != NULL; tmp = tmp->rightSibling) {
                    gencodeDeclarationNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such type of node\n");
                exit(255);
        }
    }
    gencode_closeScope();
}

void gencodeDeclarationNode(AST_NODE* declarationNode){
	switch (declarationNode->semantic_value.declSemanticValue.kind) {
        case VARIABLE_DECL:                                 // int a, b, c[];
            //declareIdList(declarationNode, false, NULL);
            break;
        case TYPE_DECL:                                     // typedef int aaa
            //declareType(declarationNode);
            break;
        case FUNCTION_DECL:                                 // int f(int a, int b){}
            gencodedeclareFunction(declarationNode);
            break;
        default:
            fprintf(stderr, "no such DECL kind\n");
            exit(255);
    }
}
void gencodedeclareFunction(AST_NODE* declarationNode){
    AST_NODE* returnType = declarationNode->child;
    AST_NODE* functionName = returnType->rightSibling;
    AST_NODE* functionParam = functionName->rightSibling;
    AST_NODE* block = functionParam->rightSibling;
	
	char* name = functionName->semantic_value.identifierSemanticValue.identifierName;
	
	
	if(!strcmp(functionName->semantic_value.identifierSemanticValue.identifierName,"MAIN")){
		fprintf(FP, ".text\n");
		fprintf(FP, "_start_MAIN:\n\n");
	}else{
		fprintf(FP, ".text\n");
		fprintf(FP, "_%s:\n\n",name);
	}
	gencodeprologue(name);
	
    gencodeBlockNode(block);
	
	gencodeepilogue(name);
}
void gencodeBlockNode(AST_NODE* blockNode){
	AST_NODE *nodeptr, *tmp;
    for(nodeptr = blockNode->child; nodeptr != NULL; nodeptr = nodeptr -> rightSibling){// 2 loops at most actually
        switch(nodeptr->nodeType) {
            case VARIABLE_DECL_LIST_NODE:                           // decl_list
                //for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {		// maybe we don't do this
                //    gencodeDeclarationNode(tmp);
                //}
                break;
            case STMT_LIST_NODE:                                    // stmt_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {
                    gencodeStmtNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] void processBlockNode(AST_NODE* blockNode).\n");
                blockNode->dataType = ERROR_TYPE;
                exit(255);
        }
    }
}

void gencodeStmtNode(AST_NODE* stmtNode){
	if(stmtNode->nodeType == BLOCK_NODE){
        gencode_openScope();
        gencodeBlockNode(stmtNode);
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
                gencodeFunctionCall(stmtNode);
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
void gencodeFunctionCall(AST_NODE* functionCallNode){
	AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    //  special case 1 and 2
    if(strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "write") == 0){
		gencodewrite(functionCallNode);
        return;
    } else if (strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "read") == 0){
		gencoderead(functionCallNode);
        return;
    } else if (strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "fread") == 0){
		gencodefread(functionCallNode);
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














void gencodeprologue(char *name){
	/*
	*/
	fprintf(FP, "addi    sp,sp,-16\n");
	fprintf(FP, "sd      ra,8(sp)\n");
	fprintf(FP, "sd      s0,0(sp)\n");
	fprintf(FP, "mv      fp,sp\n");
	fprintf(FP, "la ra, _frameSize_%s\n",name);
	fprintf(FP, "lw ra, 0(ra)\n");
	fprintf(FP, "sub sp, sp, ra\n");
	fprintf(FP, "_begin_%s:\n\n", name);
}

void gencodeepilogue(char *name){
	/*
	*/
	fprintf(FP, "mv      sp,fp\n");
	fprintf(FP, "ld      ra,8(sp)\n");
	fprintf(FP, "ld      s0,0(sp)\n");
	fprintf(FP, "addi    sp,sp,16\n");
	fprintf(FP, "jr ra\n"); 
	fprintf(FP, ".data\n");
	fprintf(FP, "_frameSize_%s: 	.word	 %d\n", name, 184);
}

void gencodefread(AST_NODE* functionCallNode){	// I don't know parameter now
	
}

void gencoderead(AST_NODE* functionCallNode){	// I don't know parameter now
	
}

void gencodewrite(AST_NODE* functionCallNode){
	AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;
	AST_NODE* param = paramList->child;
	switch (param->dataType) {
		case INT_TYPE:
			
			break;
		case FLOAT_TYPE:
			
			break;
		case CONST_STRING_TYPE:
			fprintf(FP, ".data\n");
			fprintf(FP, "_CONSTANT_%d: .ascii %s\n", constid, param->semantic_value.const1->const_u.sc);
			fprintf(FP, ".align 3\n");
			fprintf(FP, ".text\n");
			fprintf(FP, "la t0, _CONSTANT_%d\n", constid++);
			fprintf(FP, "mv a0,t0\n");
			fprintf(FP, "jal _write_str\n");
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