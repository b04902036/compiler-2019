//TODO
// 1. checkFunctionCall->checkParameterPassing->processRelopExpr->processRelopExpr
// 2. processRelopExpr and processRelopExpr should be the same function
#include "header.h"
#include "symbolTable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 document. //
extern SymbolTable symbolTable;
int g_anyErrorOccur = 0;


DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
int getExprType(AST_NODE* exprNode);
DATA_TYPE checkType(const char name[]);
DATA_TYPE processRelopExpr(AST_NODE* variable);

void processProgramNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, bool ignoreFirstDimensionOfArray, Parameter* paramList);
void declareFunction(AST_NODE* returnTypeNode);
void declareType(AST_NODE* typeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void checkWhileStmt(AST_NODE* whileNode);
void checkForStmt(AST_NODE* forNode);
void checkAssignmentStmt(AST_NODE* assignmentNode);
void checkIfStmt(AST_NODE* ifNode);
void checkWriteFunction(AST_NODE* functionCallNode);
void checkFunctionCall(AST_NODE* functionCallNode);
void checkParameterPassing(AST_NODE* passedParameter, Parameter* expectParam, char* name);
void checkReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void evaluateExprValue(AST_NODE* exprNode);


typedef enum ErrorMsgKind
{
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARE,
    SYMBOL_UNDECLARED,
    NOT_FUNCTION_NAME,
    TRY_TO_INIT_ARRAY,
    EXCESSIVE_ARRAY_DIM_DECLARATION,
    RETURN_ARRAY,
    VOID_VARIABLE,
    TYPEDEF_VOID_ARRAY,
    PARAMETER_TYPE_UNMATCH,
    TOO_FEW_ARGUMENTS,
    TOO_MANY_ARGUMENTS,
    RETURN_TYPE_UNMATCH,
    INCOMPATIBLE_ARRAY_DIMENSION,
    NOT_ASSIGNABLE,
    NOT_ARRAY,
    IS_TYPE_NOT_VARIABLE,
    IS_FUNCTION_NOT_VARIABLE,
    VOID_OPERATION,/*added*/
    STRING_OPERATION,
    ARRAY_SIZE_MISSING,/*added*/
    ARRAY_SIZE_NOT_INT,
    ARRAY_SIZE_NEGATIVE,
    ARRAY_SUBSCRIPT_NOT_INT,
    PASS_ARRAY_TO_SCALAR,
    PASS_SCALAR_TO_ARRAY
} ErrorMsgKind;


void printErrorMsgSpecial(AST_NODE* node1, char* name2, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node1->linenumber);
    switch(errorMsgKind) {
        case SYMBOL_IS_NOT_TYPE:
            printf("unknown type name %s\n", name2);
            break;
        case SYMBOL_UNDECLARED:
            printf("symbol %s undeclared\n", name2); 
            break;
        case SYMBOL_REDECLARE:
            printf("symbol %s redeclared\n", name2);
            break;
        case IS_FUNCTION_NOT_VARIABLE:
            printf("symbol %s is function, not variable\n", name2);
        case TOO_FEW_ARGUMENTS:
            printf("too few arguments to function %s\n", name2);
            break;
        case TOO_MANY_ARGUMENTS:
            printf("too many arguments to function %s\n", name2);
            break;
        case PARAMETER_TYPE_UNMATCH:
            printf("parameter pass to function %s didn't match the function signature\n", name2);
            break;
        case NOT_FUNCTION_NAME:
            printf("%s is not a function name\n", name2);
            break;
        default:
            printf("Unhandled case in void printErrorMsgSpecial(AST_NODE* node, ERROR_MSG_KIND* %d)\n", errorMsgKind);
            break;
    }
}


void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind) {
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    switch (errorMsgKind) {
        case ARRAY_SIZE_MISSING:
            printf("array size missing\n");
            break;
        case ARRAY_SIZE_NOT_INT:
            printf("array size should be int\n");
            break;
        case VOID_OPERATION:
            printf("operation should not be done on variable with void type\n");
            break;
        case STRING_OPERATION:
            printf("should not operate on string\n");
            break;
        case VOID_VARIABLE:
            printf("variable declared with type void\n");
            break;
        case INCOMPATIBLE_ARRAY_DIMENSION:
            printf("array subscription exceed array dimension\n");
            break;
        case PARAMETER_TYPE_UNMATCH:
            printf("parameter passed to function type mismatch\n");
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* %d)\n", errorMsgKind);
            break;
    }
}


void semanticAnalysis(AST_NODE *root)
{
    processProgramNode(root);
}

/**
 * Check if this relop_expr is legal, and return type of this expression
 */
DATA_TYPE processRelopExpr(AST_NODE* variable) {
    SymbolTableEntry* symbol;
    DATA_TYPE dataType1, dataType2;
    switch (variable->nodeType) {
        case CONST_VALUE_NODE:
            switch (variable->semantic_value.const1->const_type) {
                case INTEGERC:
                    return INT_TYPE;
                case FLOATC:
                    return FLOAT_TYPE;
                case STRINGC:
                    return CONST_STRING_TYPE;
                default:
                    fprintf(stderr, "[PARSER ERROR] no such constant type\n");
                    exit(255);
            }
        case EXPR_NODE:
            switch (variable->semantic_value.exprSemanticValue.kind) {
                case BINARY_OPERATION:
                    dataType1 = processRelopExpr(variable->child);
                    dataType2 = processRelopExpr(variable->child->rightSibling);
                    if (dataType1 == VOID_TYPE || dataType2 == VOID_TYPE) {
                        printErrorMsg(variable, VOID_OPERATION);
                        return ERROR_TYPE;
                    }
                    if (dataType1 == ERROR_TYPE || dataType1 == NONE_TYPE 
                                || dataType2 == ERROR_TYPE || dataType2 == NONE_TYPE) {
                        return ERROR_TYPE;
                    }

                    // if this operation is rel_op, then it will return 1 or 0
                    switch (variable->semantic_value.exprSemanticValue.op.binaryOp) {
                        case BINARY_OP_EQ:
                        case BINARY_OP_GE:
                        case BINARY_OP_LE:
                        case BINARY_OP_NE:
                        case BINARY_OP_GT:
                        case BINARY_OP_LT:
                            return INT_TYPE;
                        default:
                            break;
                    }

                    // otherwise check if this is operation between string
                    // also change pointer type to int
                    switch (dataType1) {
                        case CONST_STRING_TYPE:
                            printErrorMsg(variable, STRING_OPERATION);
                            return ERROR_TYPE;
                        case INT_PTR_TYPE:
                        case FLOAT_PTR_TYPE:
                            dataType1 = INT_TYPE;
                        default:
                            break;
                    }

                    switch (dataType2) {
                        case CONST_STRING_TYPE:
                            printErrorMsg(variable, STRING_OPERATION);
                            return ERROR_TYPE;
                        case INT_PTR_TYPE:
                        case FLOAT_PTR_TYPE:
                            dataType1 = INT_TYPE;
                        default:
                            break;
                    }
                    
                    return getBiggerType(dataType1, dataType2);
                case UNARY_OPERATION:
                    /**
                     * there are only two kinds of unary operation : "!" and "-"
                     * we allowed "!" on string constant and it will return INT_TYPE
                     */
                    dataType1 = processRelopExpr(variable);
                    switch (dataType1) {
                        case VOID_TYPE:
                            printErrorMsg(variable, VOID_OPERATION);
                            return ERROR_TYPE;
                        case CONST_STRING_TYPE:
                            if (variable->semantic_value.exprSemanticValue.op.unaryOp == UNARY_OP_NEGATIVE) {
                                printErrorMsg(variable, STRING_OPERATION);
                                return ERROR_TYPE;
                            }
                            return INT_TYPE;
                        case INT_PTR_TYPE:
                        case FLOAT_PTR_TYPE:
                        case INT_TYPE:
                            return INT_TYPE;
                        case FLOAT_TYPE:
                            if (variable->semantic_value.exprSemanticValue.op.unaryOp == UNARY_OP_NEGATIVE) {
                                return FLOAT_TYPE;
                            }
                            return INT_TYPE;
                        default:
                            return ERROR_TYPE;
                    }

                default:
                    fprintf(stderr, "[PARSER ERROR] no such type of expr\n");
                    exit(255);
            }
        case IDENTIFIER_NODE:
            symbol = retrieveSymbol(variable->semantic_value.identifierSemanticValue.identifierName, 
                                /*onlyInCurrentScope*/false);
            if (symbol == NULL) {
                printErrorMsgSpecial(variable, variable->semantic_value.identifierSemanticValue.identifierName,
                                    SYMBOL_UNDECLARED);
                return -1;
            }
            
            /**
             * if it is function pointer, print error
             * otherwise if it is scalar, return it
             * otherwise we check if it is an array slice
             */
            if (symbol->attribute->attributeKind == FUNCTION_SIGNATURE) {
                printErrorMsgSpecial(variable, symbol->name, IS_FUNCTION_NOT_VARIABLE);
            }
            else if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
                return symbol->attribute->attr.typeDescriptor->properties.dataType;
            }
            else {
                int indexCount = 0;
                int maxIndexCount = symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
                DATA_TYPE indexType;
                AST_NODE* index = variable->child;
                while (index != NULL) {
                    ++indexCount;
                    indexType = processRelopExpr(index);
                    if (indexType != INT_TYPE) {
                        printErrorMsg(index, ARRAY_SUBSCRIPT_NOT_INT);
                    }
                    index = index->rightSibling;
                }
                if (indexCount < maxIndexCount) {
                    return INT_PTR_TYPE;
                }
                else if (indexCount == maxIndexCount) {
                    return symbol->attribute->attr.typeDescriptor->properties.dataType;
                }
                else {
                    printErrorMsg(variable, INCOMPATIBLE_ARRAY_DIMENSION);
                    return -1;
                }
            }
        case STMT_NODE:
            if (variable->semantic_value.stmtSemanticValue.kind != FUNCTION_CALL_STMT) {
                fprintf(stderr, "[PARSER ERROR] should only contain function call statement here\n");
                exit(255);
            }
            variable = variable->child;
            
            /**
             * function can only return scalar or void
             * get function signature and check passed parameter
             */
            symbol = retrieveSymbol(variable->semantic_value.identifierSemanticValue.identifierName, 
                                /*onlyInCurrentScope*/false);
            if (symbol == NULL) {
                printErrorMsgSpecial(variable, variable->semantic_value.identifierSemanticValue.identifierName,
                                        SYMBOL_UNDECLARED);
            }
            else if (symbol->attribute->attributeKind != FUNCTION_SIGNATURE) {
                printErrorMsgSpecial(variable, symbol->name, NOT_FUNCTION_NAME);
            }
            else if (symbol->attribute->attr.functionSignature->returnType == VOID_TYPE) {
                printErrorMsgSpecial(variable, symbol->name, PARAMETER_TYPE_UNMATCH);
            }
            checkParameterPassing(variable->rightSibling, symbol->attribute->attr.functionSignature->parameterList, symbol->name);
            return SCALAR_TYPE_DESCRIPTOR;
    }
}

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2) {
    if(dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {
        return FLOAT_TYPE;
    }
    else {
        return INT_TYPE;
    }
}

/**
 * only use on cexpr
 * propagate type of an exprNode from bottom to top
 * use DFS here
 * return INT_TYPE or FLOAT_TYPE
 */
int getExprType(AST_NODE* exprNode) {
    switch (exprNode->nodeType) {
        case CONST_VALUE_NODE:
            switch (exprNode->semantic_value.const1->const_type) {
                case INTEGERC:
                    return INT_TYPE;
                case FLOATC:
                    return FLOAT_TYPE;
                default:
                    fprintf(stderr, "[PARSER ERROR] don't expect to get this const value node type\n");
                    exit(255);
            }
        case EXPR_NODE:
            switch (exprNode->semantic_value.exprSemanticValue.kind) {
                case BINARY_OPERATION:
                    return getBiggerType(getExprType(exprNode->child), getExprType(exprNode->child->rightSibling));
                case UNARY_OPERATION:
                    return getExprType(exprNode->child);
                default:
                    fprintf(stderr, "[PARSER ERROR] no such operation kind\n");
                    exit(255);
            }
        default:
            fprintf(stderr, "[PARSER ERROR] wrong node appear in expr");
            exit(255);
    }
}

/**
 * check if this type is 
 * 1. int
 * 2. float
 * 3. void
 * return ERROR_TYPE if not found or this type is a variable
 */
DATA_TYPE checkType (const char name[]) {
    // first check if this is "int", "float" or "void"
    if (!strcmp(name, "int")) {
        return INT_TYPE;
    }
    else if (!strcmp(name, "float")) {
        return FLOAT_TYPE;
    }
    else if (!strcmp(name, "void")) {
        return VOID_TYPE;
    }

    // now search it in symbolTable
    SymbolTableEntry* symbol = retrieveSymbol(name, /*onlyInCurrentScope*/false);
    if (symbol == NULL) {
        return ERROR_TYPE;
    }
    else if (symbol->attribute->attributeKind != TYPE_ATTRIBUTE){
        return ERROR_TYPE;
    }
    else {
        return symbol->attribute->attr.typeDescriptor->properties.dataType;
    }
}



void processProgramNode(AST_NODE *programNode) {
    AST_NODE* start = programNode->child;
    AST_NODE* tmp;
    
    // first open a scope for global declaration
    openScope();

    for (start = programNode->child; start != NULL; start = start->rightSibling) {
        switch (start->nodeType) {
            case DECLARATION_NODE:
                processDeclarationNode(start);
                break;
            case VARIABLE_DECL_LIST_NODE:
                for (tmp = start->child; tmp != NULL; tmp = tmp->rightSibling) {
                    processDeclarationNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such type of node\n");
                exit(255);
        }
    }
    fprintf(stderr, "finish processing program node\n"); 
}

void processDeclarationNode(AST_NODE* declarationNode) {
    switch (declarationNode->semantic_value.declSemanticValue.kind) {
        case VARIABLE_DECL:
            declareIdList(declarationNode, false, NULL);
            break;
        case TYPE_DECL:
            declareType(declarationNode);
            break;
        case FUNCTION_DECL:
            declareFunction(declarationNode);
            break;
        default:
            fprintf(stderr, "no such DECL kind\n");
            exit(255);
    }
}

void declareFunction(AST_NODE* declarationNode) {
    AST_NODE* returnType = declarationNode->child;
    AST_NODE* functionName = returnType->rightSibling;
    AST_NODE* functionParam = functionName->rightSibling;
    Parameter* parameterList;
    DATA_TYPE type;
    SymbolTableEntry* symbol;
    int parameterCount = 0;
    

    // first we need to check if this function name is used
    symbol = retrieveSymbol(functionName->semantic_value.identifierSemanticValue.identifierName, 
                        /*onlyInCurrentScope*/false);
    if (symbol != NULL) {
        printErrorMsgSpecial(functionName, symbol->name, SYMBOL_REDECLARE);
    }
    else {
        /**
         * remember this function name is declared out of the function body
         * so we have to allocate new symbol "before" we open a new scope for function body
         */
        symbol = newSymbolTableEntry();
        openScope();

        // set name, should not overflow as long as ID name is set correctly in parser
        symbol->name = strdup(functionName->semantic_value.identifierSemanticValue.identifierName);
        if (symbol->name == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for variable name\n");
            exit(255);
        }

        // allocate memory for symbol->attribute
        symbol->attribute = (SymbolAttribute*) malloc(sizeof(SymbolAttribute));
        if (symbol->attribute == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for symbol attribute\n");
            exit(255);
        }
        symbol->attribute->attributeKind = FUNCTION_SIGNATURE;
        
        // allocate for typeDescriptor
        symbol->attribute->attr.functionSignature = (FunctionSignature*) malloc(sizeof(FunctionSignature));
        if (symbol->attribute->attr.typeDescriptor == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for symbol attribute\n");
            exit(255);
        }
        type = checkType(returnType->semantic_value.identifierSemanticValue.identifierName);
        if (type == ERROR_TYPE) {
            printErrorMsgSpecial(returnType, returnType->semantic_value.identifierSemanticValue.identifierName,
                            SYMBOL_IS_NOT_TYPE);
        }
        symbol->attribute->attr.functionSignature->returnType = type;
        symbol->attribute->attr.functionSignature->parameterList = NULL;

        for (functionParam = functionParam->child; functionParam != NULL; functionParam = functionParam->rightSibling) {
            if (parameterCount == 0) {
                parameterList = (Parameter*) malloc(sizeof(Parameter));
                if (parameterList == NULL) {
                    fprintf(stderr, "[SEMANTIC ERROR] fail to allocate memory for parameterlist\n");
                    exit(255);
                }
                symbol->attribute->attr.functionSignature->parameterList = parameterList;
            }
            else {
                parameterList->next = (Parameter*) malloc(sizeof(Parameter));
                if (parameterList->next == NULL) {
                    fprintf(stderr, "[SEMANTIC ERROR] fail to allocate memory for parameterlist\n");
                    exit(255);
                }
                parameterList = parameterList->next;
            }
            declareIdList(functionParam, /*ignoreFirstDimensionOfArray*/true, parameterList);
            ++parameterCount;
        }
        symbol->attribute->attr.functionSignature->parametersCount = parameterCount;

        // now symbol is ready, add this into symbolTable
        addSymbol(symbol);

        // leaving, close scope
        closeScope();
    }
}

void declareType(AST_NODE* typeNode) {
    int idx = 0;
    DATA_TYPE type;
    AST_NODE* dataType = typeNode->child;
    
    /**
     * type can only be int, float or void
     */
    type = checkType(dataType->semantic_value.identifierSemanticValue.identifierName);
    if (type == ERROR_TYPE) {
        printErrorMsgSpecial(dataType, dataType->semantic_value.identifierSemanticValue.identifierName,
                        SYMBOL_IS_NOT_TYPE);
    }

    for (AST_NODE* alias = dataType->rightSibling; alias != NULL; alias = alias->rightSibling) {
        // first check if this name already exist in this scope
        if (retrieveSymbol(alias->semantic_value.identifierSemanticValue.identifierName,
                    /*onlyInCurrentScope*/true) != NULL) {
            printErrorMsgSpecial(alias, alias->semantic_value.identifierSemanticValue.identifierName,
                                SYMBOL_REDECLARE);
            continue;
        }

        // allocate new symbol entry
        SymbolTableEntry* symbol = newSymbolTableEntry();
        symbol->name = strdup(alias->semantic_value.identifierSemanticValue.identifierName);
        if (symbol->name == NULL) {
            fprintf(stderr, "[SEMANTIC ERROR] error allocating memory for symbol name\n");
            exit(255);
        }
        
        // allocate for attribute
        symbol->attribute = (SymbolAttribute*) malloc(sizeof(SymbolAttribute));
        if (symbol->attribute == NULL) {
            fprintf(stderr, "[SEMANTIC ERROR] error allocating memory for symbol attribute\n");
            exit(255);
        }
        symbol->attribute->attributeKind = TYPE_ATTRIBUTE;

        // allocate for typeDescriptor
        symbol->attribute->attr.typeDescriptor = (TypeDescriptor*) malloc(sizeof(TypeDescriptor));
        if (symbol->attribute->attr.typeDescriptor == NULL) {
            fprintf(stderr, "[SEMANTIC ERROR] error allocating memory for symbol typeDescriptor\n");
            exit(255);
        }
        symbol->attribute->attr.typeDescriptor->properties.dataType = type;

        // add this symbol to symbolTable
        addSymbol(symbol);
    }
    
}

/**
 * process Declare Identifier List
 * this is either under
 *  1. VARIABLE_DECL
 *  2. FUNCTION_PARAMETER_DECL
 * the only difference is in FUNCTION_PARAMETER_DECL, we can ignore first dimension of array definition
 */
void declareIdList(AST_NODE* declarationNode, bool ignoreFirstDimensionOfArray, Parameter* paramList) {
    int idx = 0;
    DATA_TYPE type;
    AST_NODE* dataType = declarationNode->child;
    
    /**
     * get declared type
     * this type name should not be a variable name
     */
    type = checkType(dataType->semantic_value.identifierSemanticValue.identifierName);
    if (type == ERROR_TYPE) {
        printErrorMsgSpecial(declarationNode, dataType->semantic_value.identifierSemanticValue.identifierName,
                        SYMBOL_IS_NOT_TYPE);
    }
    
    /**
     * a variable should not be declared as void type
     */
    if (type == VOID_TYPE) {
        printErrorMsg(declarationNode, VOID_VARIABLE);
    }

    for (AST_NODE* variable = dataType->rightSibling; variable != NULL; variable = variable->rightSibling) {
        // first check if this name is already used in this scope
        if (retrieveSymbol(variable->semantic_value.identifierSemanticValue.identifierName,
                    /*onlyInCurrentScope*/true) != NULL) {
            printErrorMsgSpecial(variable, variable->semantic_value.identifierSemanticValue.identifierName,
                                SYMBOL_REDECLARE);
            continue;
        }
        
        // allocate for new symbol
        SymbolTableEntry* symbol = newSymbolTableEntry();
        
        // set name, should not overflow as long as ID name is set correctly in parser
        symbol->name = strdup(variable->semantic_value.identifierSemanticValue.identifierName);
        if (symbol->name == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for variable name\n");
            exit(255);
        }

        // allocate memory for symbol->attribute
        symbol->attribute = (SymbolAttribute*) malloc(sizeof(SymbolAttribute));
        if (symbol->attribute == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for symbol attribute\n");
            exit(255);
        }
        
        // allocate for typeDescriptor
        symbol->attribute->attr.typeDescriptor = (TypeDescriptor*) malloc(sizeof(TypeDescriptor));
        if (symbol->attribute->attr.typeDescriptor == NULL) {
            fprintf(stderr, "[SEMANTIC CHECK ERROR] fail to allocate memory for symbol attribute\n");
            exit(255);
        }

        // check if it is array or scalar
        switch (variable->semantic_value.identifierSemanticValue.kind) {
            case NORMAL_ID:
                symbol->attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
                symbol->attribute->attr.typeDescriptor->properties.dataType = type;
                break;
            case ARRAY_ID:
                symbol->attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                
                /**
                 * set array dimension information
                 *  we don't care about array size in each dimension in this hw
                 *  only need to check
                 *   1. NULL (e.g. int a[][10];) is not allowed when declaring
                 *   2. float (e.g. int a[10-0.3];) is not allowed when declaring
                 */
                for (AST_NODE* arrayDimension = variable->child;
                        arrayDimension != NULL; arrayDimension = arrayDimension->rightSibling, ++idx) {
                    if (arrayDimension->nodeType == NUL_NODE && 
                            !(idx == 0 && ignoreFirstDimensionOfArray)) {
                        printErrorMsg(arrayDimension, ARRAY_SIZE_MISSING);
                    }
                    else if ((arrayDimension->nodeType == EXPR_NODE) &&
                                (getExprType(arrayDimension) == FLOAT_TYPE)) {
                        printErrorMsg(arrayDimension, ARRAY_SIZE_NOT_INT);
                    }
                    else if ((arrayDimension->nodeType == CONST_VALUE_NODE) &&
                                arrayDimension->semantic_value.const1->const_type == FLOATC) {
                        printErrorMsg(arrayDimension, ARRAY_SIZE_NOT_INT);
                    }
                }
                symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension = idx;
                symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType = type;
                break;
            case WITH_INIT_ID:
                symbol->attribute->attr.typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
                symbol->attribute->attr.typeDescriptor->properties.dataType = type;
                processRelopExpr(variable->child);
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
        } // switch, variable type

        /**
         * now symbolTableEntry is ready
         * we need to add it into symbolTable
         */
        addSymbol(symbol);

        /**
         * if ignoreFirstDimensionOfArray is true, means we are dealing with function parameter
         * and each type should not be followed with two or more variables
         * and we need to add this symbol to parameter list of this function
         */
        if (ignoreFirstDimensionOfArray) {
            // set parameter name
            paramList->parameterName = strdup(symbol->name);
            if (paramList->parameterName == NULL) {
                fprintf(stderr, "[SEMANTIC ERROR] fail to allocate memory for parameter name\n");
                exit(255);
            }
            
            // set parameter type
            paramList->type = (TypeDescriptor*) malloc(sizeof(TypeDescriptor));
            if (paramList->type == NULL) {
                fprintf(stderr, "[SEMANTIC ERROR] fail to allocate memory for parameter type\n");
                exit(255);
            }
            paramList->type->kind = symbol->attribute->attr.typeDescriptor->kind;
            if (paramList->type->kind == SCALAR_TYPE_DESCRIPTOR) {
                paramList->type->properties.dataType = symbol->attribute->attr.typeDescriptor->properties.dataType;
            }
            else {
                paramList->type->properties.arrayProperties = symbol->attribute->attr.typeDescriptor->properties.arrayProperties;
            }

            paramList->next = NULL;
            break;
        }
    } // for, over variable
}
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode) {
}

void checkWhileStmt(AST_NODE* whileNode)
{
}


void checkForStmt(AST_NODE* forNode)
{
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
}


void checkIfStmt(AST_NODE* ifNode)
{
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
}

void checkFunctionCall(AST_NODE* functionCallNode) {
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    // get function signature
    SymbolTableEntry* function = retrieveSymbol(functionName->semantic_value.identifierSemanticValue.identifierName,
                                /*onlyInCurrentScope*/false);
    if (function == NULL) {
        printErrorMsgSpecial(functionCallNode, functionName->semantic_value.identifierSemanticValue.identifierName,
                                SYMBOL_UNDECLARED);
        return;
    }
    checkParameterPassing(paramList, function->attribute->attr.functionSignature->parameterList, function->name);
}

void checkParameterPassing(AST_NODE* passedParameter, Parameter* expectParam, char* name) {
    DATA_TYPE paramType;
    int translatedType;

    if (passedParameter->nodeType == NUL_NODE &&
            expectParam == NULL) {
        return;
    }
    else if (passedParameter->nodeType == NUL_NODE) {
        printErrorMsgSpecial(passedParameter, name, TOO_FEW_ARGUMENTS);
        return;
    }
    else if (passedParameter->nodeType == NONEMPTY_RELOP_EXPR_LIST_NODE) {
        // check if type is correct
        AST_NODE* param = passedParameter->child;

        for (; expectParam != NULL; expectParam = expectParam->next) {
            if (param == NULL) {
                printErrorMsgSpecial(passedParameter, name, TOO_FEW_ARGUMENTS);
                return;
            }
            
            paramType = processRelopExpr(param);
            switch (paramType) {
                case INT_TYPE:
                case FLOAT_TYPE:
                    translatedType = SCALAR_TYPE_DESCRIPTOR;
                    break;
                case INT_PTR_TYPE:
                case FLOAT_PTR_TYPE:
                    translatedType = ARRAY_TYPE_DESCRIPTOR;
                    break;
                default:
                    translatedType = -1;
                    break;
            }
            if (translatedType != expectParam->type->kind) {
                printErrorMsg(param, PARAMETER_TYPE_UNMATCH);
            }
            param = param->rightSibling;
        }

        // param should be NULL now, if not ...
        if (param != NULL) {
            printErrorMsgSpecial(param, name, TOO_MANY_ARGUMENTS);
        }
    }
    else {
        fprintf(stderr, "[PARSER ERROR] no such function parameter type\n");
        exit(255);
    }

}


void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}


void processExprNode(AST_NODE* exprNode)
{
}


void processVariableLValue(AST_NODE* idNode)
{
}

void processVariableRValue(AST_NODE* idNode)
{
}


void processConstValueNode(AST_NODE* constValueNode)
{
}


void checkReturnStmt(AST_NODE* returnNode)
{
}


void processBlockNode(AST_NODE* blockNode)
{
}


void processStmtNode(AST_NODE* stmtNode)
{
}


void processGeneralNode(AST_NODE *node)
{
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}



