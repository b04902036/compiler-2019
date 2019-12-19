// TODO :
//      1.Array
//      2.Expression Related Operation
// I don't want to do any register tracking.
//      1. all variable should load from memory
//      2. when call function, save those using register.
//      3. occupied register only when process relop or something like that.

#include "header.h"
#include "symbolTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE *FP;
int constid = 0;
int controlid = 0;
int global_offset = 0;
int frame_offset = 0;
bool global_variable_declaration = true;

void gencode(AST_NODE* node);
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
void gencode_AssignmentStmt(AST_NODE* assignmentNode);

void gencode_ForStmt(AST_NODE* forNode);
void gencode_WhileStmt(AST_NODE* whileNode);
void gencode_IfStmt(AST_NODE* ifNode);
void gencode_AssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void gencode_ReturnStmt(AST_NODE* returnNode);

void gencode_prologue(char *name);
void gencode_epilogue(char *name);
bool is_int_reg(int regnum);
void callee_store();
void callee_restore();
void caller_store();
void caller_restore();
void gencode_read(AST_NODE* idNode);
void gencode_fread(AST_NODE* idNode);
void gencode_write(AST_NODE* functionCallNode);

// I assume that no (a1+(a2+(a3+(a4+(a5+(...))))))
// Only need in expression in function, store at those ast nodes.
//     t0-t6,  s2-s11:  int register
//  ft0-ft11,fs2-fs11:  float register
//        a0/     fa0:  return value
//     a2-a7/ fa2-fa7:  parameter passing
bool reg_arr[51];
const char regmap[51][5] = {"t0","t1","t2","t3","t4","t5","t6",                                         // 0 - 16
                            "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",                        //
                            "ft0","ft1","ft2","ft3","ft4","ft5","ft6","ft7","ft8","ft9","ft10","ft11",  //17 - 38
                            "fs2","fs3","fs4","fs5","fs6","fs7","fs8","fs9","fs10","fs11",              //
                            "a2","a3","a4","a5","a6","a7",                                              //39 - 44
                            "fa2","fa3","fa4","fa5","fa6","fa7"};                                       //45 - 50
void init_register();
int get_register(bool getint, bool parameter);
int free_register(int id);
void check_s11();               // check when load address s11
void check_register_notused();  // just for safety check

void int_2_float(AST_NODE* node);
void float_2_int(AST_NODE* node);



void gencode(AST_NODE* node){
    FP = fopen("output.s", "w");
    
    fprintf(FP, ".data\n");
    fprintf(FP, "_endline: .ascii \"\\n\\000\"\n");
    fprintf(FP, ".align 3\n");
    
    gencode_Program(node);
    
    // TODO : Actually, we should free the space for Hashtable pointer.
    
    fclose(FP);
}

void gencode_Program(AST_NODE* programNode){
    AST_NODE* start = programNode->child;
    AST_NODE* tmp;
    
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
                fprintf(stderr, "Program Node Error\n");
                exit(255);
        }
    }
    gencode_closeScope();
}

// In gencode_RelopExpr, the return value should be placed at register(AST can access).
DATA_TYPE gencode_RelopExpr(AST_NODE* variable) {
    if (variable->nodeType == EXPR_NODE) {
        gencode_ExprNode(variable);
    } else if (variable->nodeType == STMT_NODE) {
        gencode_FunctionCall(variable);        
    } else if (variable->nodeType == IDENTIFIER_NODE) {
        gencode_VariableRValue(variable);    
    } else if (variable->nodeType == CONST_VALUE_NODE) {
        gencode_ConstValueNode(variable);    
    }
    return variable->dataType;
}

void gencode_ExprNode(AST_NODE* exprNode){  // reuse register
    DATA_TYPE dataType1, dataType2;
    if (exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
        AST_NODE *left = exprNode->child;
        AST_NODE *right = exprNode->child->rightSibling;
        dataType1 = gencode_RelopExpr(left);
        dataType2 = gencode_RelopExpr(right);
        
        if (dataType1 != INT_TYPE && dataType1 != FLOAT_TYPE) {
            fprintf(stderr, "ExprNode binary node whose left child is not int or float.\n");
            exit(255);
        }
        if (dataType2 != INT_TYPE && dataType2 != FLOAT_TYPE) {
            fprintf(stderr, "ExprNode binary node whose right child is not int or float.\n");
            exit(255);
        }
        
        if (dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {        // at least one is float
            if (dataType1 == INT_TYPE) {
                int_2_float(left);
            }
            if (dataType2 == INT_TYPE) {
                int_2_float(right);
            }
            // from now on, both are float type
            const char *regleft = regmap[left->reg_place];
            const char *regright = regmap[right->reg_place];
            const char *regparent = regleft;
            free_register(right->reg_place);
            exprNode->reg_place = left->reg_place;
            switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
                case BINARY_OP_ADD:
                    fprintf(FP, "fadd.s   %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_SUB:
                    fprintf(FP, "fsub.s   %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_MUL:
                    fprintf(FP, "fmul.s   %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_DIV:
                    fprintf(FP, "fdiv.s   %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_EQ:
                    break;
                case BINARY_OP_GE:
                    break;
                case BINARY_OP_LE:
                    break;
                case BINARY_OP_NE:
                    break;
                case BINARY_OP_GT:
                    break;
                case BINARY_OP_LT:
                    break;
                case BINARY_OP_AND:
                    break;
                case BINARY_OP_OR:
                    break;
                default:
                    fprintf(stderr, "Unknown Binary Operator.\n");
                    exit(255);
            }
            
        } else if (dataType1 == INT_TYPE && dataType2 == INT_TYPE) {     // both are int
            const char *regleft = regmap[left->reg_place];
            const char *regright = regmap[right->reg_place];
            const char *regparent = regleft;
            free_register(right->reg_place);
            exprNode->reg_place = left->reg_place;
            switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp) {
                case BINARY_OP_ADD:
                    fprintf(FP, "add     %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_SUB:
                    fprintf(FP, "sub     %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_MUL:
                    fprintf(FP, "mul     %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_DIV:
                    fprintf(FP, "div     %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_EQ:
                    break;
                case BINARY_OP_GE:
                    break;
                case BINARY_OP_LE:
                    break;
                case BINARY_OP_NE:
                    break;
                case BINARY_OP_GT:
                    fprintf(FP, "slt     %s, %s, %s\n", regparent, regright, regleft);
                    break;
                case BINARY_OP_LT:
                    fprintf(FP, "slt     %s, %s, %s\n", regparent, regleft, regright);
                    break;
                case BINARY_OP_AND:
                    break;
                case BINARY_OP_OR:
                    break;
                default:
                    fprintf(stderr, "Unknown Binary Operator.\n");
                    exit(255);
            }
            
        }
        
    } else if (exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION) {
        AST_NODE *child = exprNode->child;
        dataType1 = gencode_RelopExpr(child);
        
        if (dataType1 == FLOAT_TYPE) {
            const char *regchild = regmap[child->reg_place];
            exprNode->reg_place = child->reg_place;
            switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
                case UNARY_OP_POSITIVE:
                    break;
                case UNARY_OP_NEGATIVE:
                    fprintf(FP, "fneg.s  %s, %s\n", regchild, regchild);
                    break;
                case UNARY_OP_LOGICAL_NEGATION: // how 2
                    break;
                default:
                    fprintf(stderr, "Unknown Unary Operator.\n");
                    exit(255);
            }
        } else if(dataType1 == INT_TYPE) {
            const char *regchild = regmap[child->reg_place];
            exprNode->reg_place = child->reg_place;
            switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp) {
                case UNARY_OP_POSITIVE:
                    break;
                case UNARY_OP_NEGATIVE:
                    fprintf(FP, "neg     %s, %s\n", regchild, regchild);
                    break;
                case UNARY_OP_LOGICAL_NEGATION: // how 2
                    break;
                default:
                    fprintf(stderr, "Unknown Unary Operator.\n");
                    exit(255);
            }
        } else {
            fprintf(stderr, "ExprNode uniry node whose child is not int or float.\n");
            exit(255);
        }
    } else {
        fprintf(stderr, "ExprNode type which is not unary or binary.\n");
        exit(255);
    }
}

void gencode_VariableRValue(AST_NODE* idNode){
    SymbolTableEntry *symbol = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, false);
    if (symbol==NULL){
        fprintf(stderr, "RValue Error.\n");
        exit(255);
    }
    
    if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        int reg_place;
        if (symbol->global == true) {    // global global
            if (symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE) {
                reg_place = get_register(true, false);
                idNode->reg_place = reg_place;
                fprintf(FP, "lw      %s,_g_%d\n", regmap[reg_place], symbol->offset);
            } else {    
                check_s11();
                reg_place = get_register(false, false);
                idNode->reg_place = reg_place;
                fprintf(FP, "la      s11,_g_%d\n", symbol->offset);
                fprintf(FP, "flw     %s,0(s11)\n", regmap[reg_place]);
            }
        } else {                        // local local
            if (symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE) {
                reg_place = get_register(true, false);
                idNode->reg_place = reg_place;
                fprintf(FP, "lw      %s,%d(sp)\n", regmap[reg_place], symbol->offset);
            } else {
                reg_place = get_register(false, false);
                idNode->reg_place = reg_place;
                fprintf(FP, "flw     %s,%d(sp)\n", regmap[reg_place], symbol->offset);
            }
        }
    }
    else {  // Array Element
        // Array Element
    }
}

void gencode_ConstValueNode(AST_NODE* constValueNode){
    int reg_place;
    switch (constValueNode->semantic_value.const1->const_type) {
        case INTEGERC:
            //fprintf(stderr, "Gencode Int.\n");
            reg_place = get_register(true, false);
            constValueNode->reg_place = reg_place;
            fprintf(FP, "li      %s,%d\n", regmap[reg_place], constValueNode->semantic_value.const1->const_u.intval);
            return;
        case FLOATC:
            //fprintf(stderr, "Gencode Float.\n");
            check_s11();
            reg_place = get_register(false, false);
            constValueNode->reg_place = reg_place;
            fprintf(FP, ".data\n");
            fprintf(FP, "_CONSTANT_%d: .float %f\n", constid, constValueNode->semantic_value.const1->const_u.fval);
            fprintf(FP, ".align 3\n");
            fprintf(FP, ".text\n");
            fprintf(FP, "la      s11, _CONSTANT_%d\n", constid++);
            fprintf(FP, "flw     %s,0(s11)\n",regmap[reg_place]);
            return;
        default:    // cannot be string, string is used only in write, I deal with it in other place.
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
            fprintf(stderr, "no such symbol \"%s\" in symbol table\n",variable->semantic_value.identifierSemanticValue.identifierName);
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
                    fprintf(FP, ".align 3\n");
                    break;
                case WITH_INIT_ID:  // !!! We only handle constant initializations "int a = 3;"
                    if(symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
                        fprintf(FP, "_g_%d:  .word %d\n",global_offset++, variable->child->semantic_value.const1->const_u.intval);
                    } else {
                        fprintf(FP, "_g_%d:  .float %f\n",global_offset++, variable->child->semantic_value.const1->const_u.fval);
                    }
                    fprintf(FP, ".align 3\n");
                    break;
                case ARRAY_ID:      // global ARRAY Declare
                    //symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                    //symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                    
                    break;
                default:
                    fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
                    exit(255);
            }
        } else {  // local declaration
            symbol->global = false;
            symbol->offset = frame_offset;
            
            switch (variable->semantic_value.identifierSemanticValue.kind) {
                case NORMAL_ID:
                    if (symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE) {
                        // nothing to do
                        frame_offset += 8;
                    } else {
                        // nothing to do
                        frame_offset += 8;
                    }
                    break;
                case WITH_INIT_ID:  // !!! We only handle constant initializations "int b = 4;"
                    if (symbol->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE) {
                        fprintf(FP, "li      t0,%d\n", variable->child->semantic_value.const1->const_u.intval);
                        fprintf(FP, "sw      t0,%d(sp)\n", frame_offset);
                        frame_offset += 8;
                    } else {
                        check_s11();
                        fprintf(FP, ".data\n");
                        fprintf(FP, "_CONSTANT_%d: .float %f\n", constid, variable->child->semantic_value.const1->const_u.fval);
                        fprintf(FP, ".align 3\n");
                        fprintf(FP, ".text\n");
                        fprintf(FP, "la      s11, _CONSTANT_%d\n", constid++);
                        fprintf(FP, "flw     ft0,0(s11)\n");
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
                    exit(255);
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
    
    frame_offset = 0;
    char* name = functionName->semantic_value.identifierSemanticValue.identifierName;
    
    if (!strcmp(functionName->semantic_value.identifierSemanticValue.identifierName,"MAIN")) {
        fprintf(FP, "\n\n\n.text\n");
        fprintf(FP, "_start_MAIN:\n");
    } else {
        fprintf(FP, "\n\n\n.text\n");
        fprintf(FP, "_start_%s:\n",name);
    }
    gencode_prologue(name);
    gencode_openScope();
    
    init_register();            // actually no needed !
    gencode_BlockNode(block);
    check_register_notused();   // when gen function block, no register should be occupied !
    
    gencode_closeScope();
    gencode_epilogue(name);
}

void gencode_BlockNode(AST_NODE* blockNode){
    AST_NODE *nodeptr, *tmp;
    for (nodeptr = blockNode->child; nodeptr != NULL; nodeptr = nodeptr -> rightSibling) {// 2 loops at most actually
        switch(nodeptr->nodeType) {
            case VARIABLE_DECL_LIST_NODE:                           // decl_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {
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
                exit(255);
        }
    }
}

void gencode_StmtNode(AST_NODE* stmtNode){
    if (stmtNode->nodeType == BLOCK_NODE) {
        gencode_openScope();
        gencode_BlockNode(stmtNode);
        gencode_closeScope();
    } else if (stmtNode->nodeType == NUL_NODE) {
        return;
    } else if (stmtNode->nodeType == STMT_NODE) {
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
                free_register(stmtNode->reg_place);     // this is disturbing
                break;
            case FUNCTION_CALL_STMT:
                gencode_FunctionCall(stmtNode);
                free_register(stmtNode->reg_place);     // this is disturbing
                break;
            case IF_STMT:
                gencode_IfStmt(stmtNode);
                break;
            case RETURN_STMT:
                gencode_ReturnStmt(stmtNode);
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] void processStmtNode(AST_NODE* stmtNode).\n");
                exit(255);
        }
    } else {
        fprintf(stderr, "[PARSER ERROR] void processStmtNode(AST_NODE* stmtNode).\n");
        exit(255);
    }
}

void gencode_FunctionCall(AST_NODE* functionCallNode){
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    // Special case : write ( I put read into gencode_assignment )
    if (strcmp(functionName->semantic_value.identifierSemanticValue.identifierName, "write") == 0) {
        gencode_write(functionCallNode);
        return;
    }
    
    
    // Assume ParameterLess,        
    if (paramList->nodeType == NUL_NODE) {
        caller_store();
        fprintf(FP, "jal     _start_%s\n", functionName->semantic_value.identifierSemanticValue.identifierName);
        caller_restore();
    } else {  // Parameter
        // Parameter
        
    }
    
    // Put Return Value
    int reg_place;
    if (functionCallNode->dataType == INT_TYPE) {    // int
        reg_place = get_register(true, false);
        functionCallNode->reg_place = reg_place;
        fprintf(FP, "mv      %s,a0\n", regmap[reg_place]);
    } else {                                        // float
        reg_place = get_register(false, false);
        functionCallNode->reg_place = reg_place;
        fprintf(FP, "fmv.s   %s,fa0\n", regmap[reg_place]);
    }
    return;
}

void gencode_AssignmentStmt(AST_NODE* assignmentNode){
    AST_NODE *var_ref = assignmentNode->child;
    AST_NODE *relop_expr = var_ref->rightSibling;

    // Special case : Read a = read();      b = fread();, I assume that only 2 case, not deep.
    if (relop_expr->nodeType == STMT_NODE && strcmp(relop_expr->child->semantic_value.identifierSemanticValue.identifierName, "read") == 0) {
        if (var_ref->dataType != INT_TYPE) {
            fprintf(stderr, "read error.\n");
            exit(255);
        }
        gencode_read(var_ref);
        return;
    } else if (relop_expr->nodeType == STMT_NODE && strcmp(relop_expr->child->semantic_value.identifierSemanticValue.identifierName, "fread") == 0) {
        if (var_ref->dataType != FLOAT_TYPE) {
            fprintf(stderr, "fread error.\n");
            exit(255);
        }
        gencode_fread(var_ref);
        return;
    }
    
    // Right Right Right Handside   --  relop_expr , if "a = b" and a's type mismatch b's
    gencode_RelopExpr(relop_expr);
    
    if (var_ref->dataType == INT_TYPE && relop_expr->dataType == FLOAT_TYPE) {
        float_2_int(relop_expr);
    } else if (var_ref->dataType == FLOAT_TYPE && relop_expr->dataType == INT_TYPE) {
        int_2_float(relop_expr);
    }
    
    // Left Left Left Handside  --  var_ref
    SymbolTableEntry *symbol = retrieveSymbol(var_ref->semantic_value.identifierSemanticValue.identifierName, /*onlyInCurrentScope*/false);
    if (symbol==NULL) {
        fprintf(stderr, "Assign Left Handside Error.\n");
        exit(255);
    }
    
    if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        if (symbol->global == true) {    // global
            check_s11();
            if(var_ref->dataType == INT_TYPE) {
                fprintf(FP, "la      s11,_g_%d\n", symbol->offset);
                fprintf(FP, "sw      %s,0(s11)\n", regmap[relop_expr->reg_place]);
            } else {    
                fprintf(FP, "la      s11,_g_%d\n", symbol->offset);
                fprintf(FP, "fsw     %s,0(s11)\n", regmap[relop_expr->reg_place]);
            }
        } else {                        // local
            if(var_ref->dataType == INT_TYPE) {
                fprintf(FP, "sw      %s,%d(sp)\n", regmap[relop_expr->reg_place], symbol->offset);
            } else {
                fprintf(FP, "fsw     %s,%d(sp)\n", regmap[relop_expr->reg_place], symbol->offset);
            }
        }
        // Who called gencode_AssignmentStmt should free register.
        // free_register(relop_expr->reg_place);
        assignmentNode->reg_place = relop_expr->reg_place;
    }
    else {  // Array Element    
        // Array Element
    }
}



void gencode_ForStmt(AST_NODE* forNode){
    
}

void gencode_WhileStmt(AST_NODE* whileNode){
    AST_NODE *test = whileNode->child;
    AST_NODE *stmt = test->rightSibling;
    
    int local_controlid = controlid++;
    
    fprintf(FP, "_Test%d:\n", local_controlid);
    gencode_AssignOrExpr(test);
    fprintf(FP, "beqz    %s, _Lexit%d\n", regmap[test->reg_place], local_controlid);
    free_register(test->reg_place);
    gencode_StmtNode(stmt);
    fprintf(FP, "j       _Test%d\n", local_controlid);
    fprintf(FP, "_Lexit%d:\n", local_controlid);
}

void gencode_IfStmt(AST_NODE* ifNode){
    AST_NODE *test = ifNode->child;
    AST_NODE *stmt1 = test->rightSibling;
    AST_NODE *stmt2 = stmt1->rightSibling;  // It will be null node but not a null pointer if no else statement.
    
    int local_controlid = controlid++;
    
    if (stmt2) {        // if "test" then "stmt1" else "stmt2"
        gencode_AssignOrExpr(test);
        fprintf(FP, "beqz    %s, Lelse%d\n", regmap[test->reg_place], local_controlid);
        gencode_StmtNode(stmt1);
        fprintf(FP, "j       Lexit%d\n", local_controlid);
        fprintf(FP, "Lelse%d:\n", local_controlid);
        gencode_StmtNode(stmt2);
        fprintf(FP, "Lexit%d:\n", local_controlid);
        free_register(test->reg_place);
    } else {            // if "test" then "stmt1"
        gencode_AssignOrExpr(test);
        fprintf(FP, "beqz    %s, Lexit%d\n", regmap[test->reg_place], local_controlid);
        gencode_StmtNode(stmt1);
        fprintf(FP, "Lexit%d:\n", local_controlid);
        free_register(test->reg_place);
    }
    
}
void gencode_AssignOrExpr(AST_NODE* assignOrExprRelatedNode) {
    if (assignOrExprRelatedNode->nodeType == STMT_NODE)  {	// Assign Statement if(a=b) ...
        gencode_AssignmentStmt(assignOrExprRelatedNode);
    }
    else {													// Relop Expression if(a+b) ...
        gencode_RelopExpr(assignOrExprRelatedNode);
    }
    
    if (assignOrExprRelatedNode->dataType == FLOAT_TYPE) {
        float_2_int(assignOrExprRelatedNode);
        assignOrExprRelatedNode->dataType = INT_TYPE;
    }
}

void gencode_ReturnStmt(AST_NODE* returnNode){// put value on a0 or fa0
    DATA_TYPE type_declaration, type_return;
    char *functionName, *returnTypeNodeName;
    
    // 1. Get the type of type_declaration
    for (AST_NODE *tmp = returnNode->parent; tmp != NULL; tmp = tmp->parent) {
        if( tmp->nodeType == DECLARATION_NODE &&
            tmp->semantic_value.declSemanticValue.kind == FUNCTION_DECL) {
                
    		AST_NODE *returnTypeNode = tmp->child;
    		returnTypeNodeName = returnTypeNode->semantic_value.identifierSemanticValue.identifierName;
            functionName = returnTypeNode->rightSibling->semantic_value.identifierSemanticValue.identifierName;

    		if ( strcmp(returnTypeNodeName,"void")==0 ) {
    			type_declaration = VOID_TYPE;
    		}
    		else if ( strcmp(returnTypeNodeName,"int")==0 ) {
    			type_declaration = INT_TYPE;
    		}
    		else if ( strcmp(returnTypeNodeName,"float")==0 ) {
    			type_declaration = FLOAT_TYPE;
    		}
    		else {		//"type" f(int a, int b){}
    			SymbolTableEntry* symbol = retrieveSymbol(returnTypeNodeName, /*onlyInCurrentScope*/false);
    			type_declaration = symbol->attribute->attr.typeDescriptor->properties.dataType;
    		}		
            
            break;
        }
    }

    // 2. Get the type of type_return
    AST_NODE *relop_expr = returnNode->child;
    if (relop_expr->nodeType == NUL_NODE) {           // return "null node"
        return;
    }
    type_return = gencode_RelopExpr(relop_expr);    // return "relop_expr" 
    
    if (type_declaration == INT_TYPE && type_return == FLOAT_TYPE) {
        float_2_int(relop_expr);
    } 
    if (type_declaration == FLOAT_TYPE && type_return == INT_TYPE) {
        int_2_float(relop_expr);
    }
    
    if (type_declaration == INT_TYPE) {
        fprintf(FP, "mv      a0,%s\n", regmap[relop_expr->reg_place]);
        free_register(relop_expr->reg_place);
    } else if(type_declaration == FLOAT_TYPE) {
        fprintf(FP, "fmv     fa0,%s\n", regmap[relop_expr->reg_place]);
        free_register(relop_expr->reg_place);
    } else {
        fprintf(stderr, "Return type not known error!\n");
        exit(255);
    }
    
    fprintf(FP, "j       _end_%s\n", functionName);
    
}








void gencode_prologue(char *name){
    fprintf(FP, "addi    sp,sp,-16\n");
    fprintf(FP, "sd      ra,8(sp)\n");
    fprintf(FP, "sd      fp,0(sp)\n");
    fprintf(FP, "mv      fp,sp\n");
    fprintf(FP, "la      ra, _frameSize_%s\n",name);
    fprintf(FP, "lw      ra, 0(ra)\n");
    fprintf(FP, "sub     sp, sp, ra\n");
    //callee_store();
    fprintf(FP, "_begin_%s:\n\n", name);
}

void gencode_epilogue(char *name){
    fprintf(FP, "\n_end_%s:\n", name);
    //callee_restore();
    fprintf(FP, "mv      sp,fp\n");
    fprintf(FP, "ld      ra,8(sp)\n");
    fprintf(FP, "ld      fp,0(sp)\n");
    fprintf(FP, "addi    sp,sp,16\n");
    fprintf(FP, "jr ra\n"); 
    fprintf(FP, ".data\n");
    fprintf(FP, "_frameSize_%s:     .word    %d\n", name, frame_offset);
}

void gencode_fread(AST_NODE* idNode){
    fprintf(FP, "call    _read_float\n");  // data in fa0
    fprintf(FP, "fmv.s   ft0,fa0\n");
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
    
    if (symbol==NULL) {
        fprintf(stderr, "fread error\n");
        exit(255);
    }
    
    if (symbol->global == true) {
        fprintf(FP, "la      s11,_g_%d\n",symbol->offset);
        fprintf(FP, "fsw     ft0,0(s11)\n");
    } else {
        fprintf(FP, "fsw     ft0,%d(sp)\n",symbol->offset);
    }
}

void gencode_read(AST_NODE* idNode){
    fprintf(FP, "call    _read_int\n");    // data in a0
    fprintf(FP, "mv      t0,a0\n");
    
    SymbolTableEntry* symbol = gencode_retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName,/*onlyInCurrentScope*/false);
    
    if (symbol==NULL) {
        fprintf(stderr, "fread error\n");
        exit(255);
    }
    
    if (symbol->global == true) {
        fprintf(FP, "la      s11,_g_%d\n",symbol->offset);
        fprintf(FP, "sw      t0,0(s11)\n");
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
    if (param->dataType == CONST_STRING_TYPE) {
        if (!strcmp(param->semantic_value.const1->const_u.sc, "\"\\n\"")) {
            fprintf(FP, "la      t0,_endline\n");
            fprintf(FP, "mv      a0,t0\n");
            fprintf(FP, "jal     _write_str\n");
            return;
        }
        //fprintf(stderr, "%s\n", param->semantic_value.const1->const_u.sc);
        fprintf(FP, ".data\n");
        
        // Actually, I don't know what its meaning, I just put \000 after the string like sample code.
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
    
    if (symbol == NULL || symbol->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {    // we assume cannot write array
        fprintf(stderr, "write function Error QQ\n");
        exit(255);
    }
    
    switch (symbol->attribute->attr.typeDescriptor->properties.dataType) {
        // Case 2 INT
        case INT_TYPE:
            if (symbol->global == true) {
                fprintf(FP, "la      s11,_g_%d\n",symbol->offset);
                fprintf(FP, "lw      t0,0(s11)\n");
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
            if (symbol->global == true) {
                fprintf(FP, "la      s11,_g_%d\n",symbol->offset);
                fprintf(FP, "flw     ft0,0(s11)\n");
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
            exit(255);
    }
}


void init_register(){
    memset(reg_arr,0,sizeof(reg_arr));
}

int get_register(bool getint, bool parameter){
    if (getint & parameter) {
        for (int i=39; i<=44;i++) {
            if (reg_arr[i]==false) {
                reg_arr[i] = true;
                return i;
            }
        }
        fprintf(stderr, "Int, Parameter register Error.\n");
        exit(255);
    } else if (getint & !parameter) {
        for (int i=0; i<=16;i++) {
            if (reg_arr[i]==false) {
                reg_arr[i] = true;
                return i;
            }
        }
        fprintf(stderr, "Int, Normal register Error.\n");
        exit(255);
    } else if (!getint & parameter) {
        for (int i=45; i<=50;i++) {
            if (reg_arr[i]==false) {
                reg_arr[i] = true;
                return i;
            }
        }
        fprintf(stderr, "Float, Parameter register Error.\n");
        exit(255);
    }else if (!getint & !parameter) {
        for (int i=17; i<=38;i++) {
            if (reg_arr[i]==false) {
                reg_arr[i] = true;
                return i;
            }
        }
        fprintf(stderr, "Float, Normal register Error.\n");
        exit(255);
    }
}

int free_register(int id){
    reg_arr[id] = false;
}

void check_s11(){
    if (reg_arr[16]==true) {
        fprintf(stderr, "No space for -s11- to load addr of const float.\n");
        exit(255);
    }
}

void check_register_notused(){
    for (int i=0; i<=50;i++) {
        if (reg_arr[i]==true) {
            fprintf(stderr, "Some register is blocked error\n");
            exit(255);
        }
    }
}


bool is_int_reg(int regnum){
    if ( 0<=regnum && regnum<=16) return true;
    if (39<=regnum && regnum<=44) return true;
    return false;
}

void callee_store(){
    // I don't want to put anything here.
}
void callee_restore(){
    // I don't want to put anything here.
}
void caller_store(){
    int num = 0;
    for (int i=0; i<=50; i++) {
        if (reg_arr[i]==true)    num++;
    }
    if (num) {
        fprintf(FP, "addi    sp,sp,-%d\n",8*num);
        
        num = 0;
        for (int i=0; i<=50; i++) {
            if (reg_arr[i]==true) {
                if (is_int_reg(i))  fprintf(FP, "sd    %s,%d(sp)\n", regmap[i], 8*num);
                else                fprintf(FP, "fsw   %s,%d(sp)\n", regmap[i], 8*num);
                num++;
            }
        }
    }
}
void caller_restore(){
    int num = 0;
    for (int i=0; i<=50; i++) {
        if (reg_arr[i]==true) {
            if (is_int_reg(i))  fprintf(FP, "ld    %s,%d(sp)\n", regmap[i], 8*num);
            else                fprintf(FP, "flw   %s,%d(sp)\n", regmap[i], 8*num);
            num++;
        }
    }
    if (num) {
        fprintf(FP, "addi    sp,sp,%d\n",8*num);
    }
}




void int_2_float(AST_NODE* node){       // int to float : fcvt.s.w ft0, t0
    int reg_place = get_register(false, false);
    fprintf(FP, "fcvt.s.w   %s, %s\n",regmap[reg_place],regmap[node->reg_place]);
    free_register(node->reg_place);
    node->reg_place = reg_place;
    return;
}
void float_2_int(AST_NODE* node){       // float to int : fcvt.w.s t0, ft0
    int reg_place = get_register(true, false);
    fprintf(FP, "fcvt.w.s   %s, %s\n",regmap[reg_place],regmap[node->reg_place]);
    free_register(node->reg_place);
    node->reg_place = reg_place;
    return;
}
