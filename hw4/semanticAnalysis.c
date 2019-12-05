// TODO
//  check function return type and declared type
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
DATA_TYPE processVariableLValue(AST_NODE* idNode);  // change return type
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


void printErrorMsgSpecial2(int linenumber, char* name1, char* name2, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", linenumber);
    switch(errorMsgKind) {
        case PASS_ARRAY_TO_SCALAR:
            printf("Array < %s > passed to scalar parameter < %s >.\n", name1, name2);
            break;
        case PASS_SCALAR_TO_ARRAY:
            printf("Scalar < %s > passed to array parameter < %s >.\n", name1, name2);
            break;
        default:
            printf("Unhandled case in void printErrorMsgSpecial2(char* name1, char* name2, ErrorMsgKind* %d)\n", errorMsgKind);
            break;
    }
}
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
            break;
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
        case IS_TYPE_NOT_VARIABLE:
            printf("%s is a type, not a varable\n", name2);
            break;
        case NOT_ARRAY:
            printf("%s is not an array\n", name2);
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
        case ARRAY_SUBSCRIPT_NOT_INT:
            printf("array index should be int\n");
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
            printf("array subscription exceed array dimension (Incompatible array dimensions.) \n");    // change
            break;
        case PARAMETER_TYPE_UNMATCH:
            printf("parameter passed to function type mismatch\n");
            break;
        case RETURN_TYPE_UNMATCH:
            printf("Incompatible return type.\n");
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
 * We need to put the datatype in to variable->dataType after calling these functions.
 * 
 * May be called from declaration, or statement.
 */
DATA_TYPE processRelopExpr(AST_NODE* variable) {
    if (variable->nodeType == EXPR_NODE) {					// INT_TYPE / FLOAT_TYPE
    	processExprNode(variable);
    } else if (variable->nodeType == STMT_NODE) {			// INT_TYPE / FLOAT_TYPE / VOID_TYPE
    	checkFunctionCall(variable);        
    } else if (variable->nodeType == IDENTIFIER_NODE) {		// INT_TYPE / FLOAT_TYPE / INT_PTR_TYPE / FLOAT_PTR_TYPE
    	processVariableRValue(variable);    
    } else if (variable->nodeType == CONST_VALUE_NODE) {	// INT_TYPE / FLOAT_TYPE / CONST_STRING_TYPE;
    	processConstValueNode(variable);    
    }
    //fprintf(stderr, "--- mark %d\n", variable->dataType);
    return variable->dataType;
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
 * 
 * TODO :
 *      1. Should we store these semantic value into AST_NODE to prevent recalculation?
 *      2. What's the relationship between processExprNode()
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
 *
 * Used in declaration
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
            case DECLARATION_NODE:                  // function declaration
                processDeclarationNode(start);
                break;
            case VARIABLE_DECL_LIST_NODE:           // decl_list 
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
    
    // added, close a scope for global declaration
    closeScope();   
}

void processDeclarationNode(AST_NODE* declarationNode) {
    switch (declarationNode->semantic_value.declSemanticValue.kind) {
        case VARIABLE_DECL:                                 // int a, b, c[];
            declareIdList(declarationNode, false, NULL);
            break;
        case TYPE_DECL:                                     // typedef int aaa
            declareType(declarationNode);
            break;
        case FUNCTION_DECL:                                 // int f(int a, int b){}
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
    AST_NODE* block = functionParam->rightSibling;
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

        openScope();
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

        // now we deal with function body
        processBlockNode(block);

        // leaving, close scope
        closeScope();
    }
}

void declareType(AST_NODE* typeNode) {
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
                idx = 0;
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
    if (assignOrExprRelatedNode->nodeType == STMT_NODE)  {	// Assign Statement
        checkAssignmentStmt(assignOrExprRelatedNode);
    }
    else {													// Relop Expression ( data type cannot stated by if statement)
        DATA_TYPE catch = processRelopExpr(assignOrExprRelatedNode);
    }
}

/* In the following 6 functions, we assume that the following functions are well worked.
 *           void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
 *      DATA_TYPE processRelopExpr(AST_NODE* variable)
 *           void processStmtNode(AST_NODE* stmtNode)
 */      
// ----------   check statment    start --------------
void checkWhileStmt(AST_NODE* whileNode)
{
    AST_NODE *test = whileNode->child;
    AST_NODE *stmt = test->rightSibling;
    checkAssignOrExpr(test);
    processStmtNode(stmt);

    if (test->nodeType == ERROR_TYPE || stmt->nodeType == ERROR_TYPE) {
        whileNode->nodeType = ERROR_TYPE;
    }
}


void checkForStmt(AST_NODE* forNode)
{
    AST_NODE *assign_expr_list1 = forNode->child;
    AST_NODE *relop_expr_list = assign_expr_list1->rightSibling;
    AST_NODE *assign_expr_list2 = relop_expr_list->rightSibling;
    AST_NODE *stmt = assign_expr_list2->rightSibling;

    AST_NODE *tmp;
    for (tmp = assign_expr_list1->child; tmp != NULL; tmp = tmp->rightSibling) {
        checkAssignOrExpr(tmp);
        if(tmp->nodeType == ERROR_TYPE)     assign_expr_list1->nodeType = ERROR_TYPE;
    }
    for (tmp = relop_expr_list->child; tmp != NULL; tmp = tmp->rightSibling) {
        DATA_TYPE catch = processRelopExpr(tmp);
        if(tmp->nodeType == ERROR_TYPE)     relop_expr_list->nodeType = ERROR_TYPE;
    }
    for (tmp = assign_expr_list2->child; tmp != NULL; tmp = tmp->rightSibling) {
        checkAssignOrExpr(tmp);
        if(tmp->nodeType == ERROR_TYPE)     assign_expr_list2->nodeType = ERROR_TYPE;
    }
    processStmtNode(stmt);

    if (assign_expr_list1->nodeType == ERROR_TYPE || relop_expr_list->nodeType == ERROR_TYPE 
        || assign_expr_list2->nodeType == ERROR_TYPE || stmt->nodeType == ERROR_TYPE ) 
    {
        forNode->nodeType = ERROR_TYPE;
    }
}

void checkAssignmentStmt(AST_NODE* assignmentNode)
{
    AST_NODE *var_ref = assignmentNode->child;
    AST_NODE *relop_expr = var_ref->rightSibling;
    
    //check for the left data type and right data type
    DATA_TYPE dataType_left = processVariableLValue(var_ref);
    DATA_TYPE dataType_right = processRelopExpr(relop_expr);
    
    bool c1 = (dataType_left == ERROR_TYPE || dataType_right == ERROR_TYPE);                // one side is illegal
    bool c2 = (dataType_left == CONST_STRING_TYPE || dataType_right == CONST_STRING_TYPE);  // string operation
    bool c3 = (dataType_right == INT_PTR_TYPE || dataType_right == FLOAT_PTR_TYPE);         // pointer assigment

    if (c1) {
        assignmentNode->nodeType = ERROR_TYPE;
    }
    else if (c2) {
        assignmentNode->nodeType = ERROR_TYPE;
        printErrorMsg(assignmentNode, STRING_OPERATION);
    }
    else if (c3) {
        assignmentNode->nodeType = ERROR_TYPE;
        printErrorMsg(assignmentNode, INCOMPATIBLE_ARRAY_DIMENSION);
    }
    else {
        assignmentNode->nodeType = getBiggerType(dataType_left, dataType_right);
    }
}


void checkIfStmt(AST_NODE* ifNode)
{
    AST_NODE *test = ifNode->child;
    AST_NODE *stmt1 = test->rightSibling;
    AST_NODE *stmt2 = stmt1->rightSibling;  // It will be null node but not a null pointer if no else statement.
    checkAssignOrExpr(test);
    processStmtNode(stmt1);
    processStmtNode(stmt2);

    if (test->nodeType == ERROR_TYPE || stmt1->nodeType == ERROR_TYPE || stmt2->nodeType == ERROR_TYPE) {
        ifNode->nodeType = ERROR_TYPE;
    }
}

// Check the match between actual return and declaration
void checkReturnStmt(AST_NODE* returnNode)
{
    AST_NODE *relop_expr = returnNode->child;   //return "null node" or "relop_expr node"    
    DATA_TYPE type_declaration, type_return;

    // 1. Get the type of type_declaration
    AST_NODE *tmp;
    for (tmp = returnNode->parent; tmp != NULL; tmp = tmp->parent) {
        if( tmp->nodeType == DECLARATION_NODE &&
            tmp->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
            
            // can't get attribute of function declaration in symbol table
            // since may the same name may be declared into variable in the function.
    		
    		AST_NODE *returnTypeNode = tmp->child;
    		char returnTypeNodeName[100];
    		strcpy(returnTypeNodeName, returnTypeNode->semantic_value.identifierSemanticValue.identifierName);

    		if ( strcmp(returnTypeNodeName,"void")==0 ) {
    			type_declaration = VOID_TYPE;
    		}
    		else if ( strcmp(returnTypeNodeName,"int")==0 ) {
    			type_declaration = INT_TYPE;
    		}
    		else if ( strcmp(returnTypeNodeName,"float")==0 ) {
    			type_declaration = FLOAT_TYPE;
    		}
    		else {		//"type" f(int a, int b){}    or ERROR
    			SymbolTableEntry* symbol = retrieveSymbol(returnTypeNodeName, /*onlyInCurrentScope*/false);
    			if (symbol == NULL) {
    				type_declaration = ERROR_TYPE;
    			} 
    			else if (symbol->attribute->attributeKind != TYPE_ATTRIBUTE) {
    				type_declaration = ERROR_TYPE;
    			}
    			else {
    				type_declaration = symbol->attribute->attr.typeDescriptor->properties.dataType;
    			}
    		}		
            
            break;
        }
    }

    // 2. Get the type of type_return
    if(relop_expr->nodeType == NUL_NODE){       // return "null node"
        if(type_declaration != VOID_TYPE){
            printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
        }
        return;
    }
    type_return = processRelopExpr(relop_expr); // return "relop_expr" 
    
    bool c1 = (type_declaration == type_return);                            //match
    bool c2 = (type_declaration == INT_TYPE && type_return == FLOAT_TYPE);  //conversion
    bool c3 = (type_declaration == FLOAT_TYPE && type_return == INT_TYPE);  //conversion
    
	fprintf(stderr, "type_declaration : %d, type_return : %d\n", type_declaration,type_return);
    
    if (!c1 && !c2 && !c3) {
        printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
    }
}


/*
 *  1.Generally, only check for the function name and parameter passing.
 *  2.Lazy checking:
 *      Should we check the return type not to be void  ->  we don't check
 */
void checkFunctionCall(AST_NODE* functionCallNode)
{
    AST_NODE* functionName = functionCallNode->child;
    AST_NODE* paramList = functionName->rightSibling;

    // get function signature
    SymbolTableEntry* function = retrieveSymbol(functionName->semantic_value.identifierSemanticValue.identifierName,
                                /*onlyInCurrentScope*/false);
    if (function == NULL) {
        printErrorMsgSpecial(functionCallNode, functionName->semantic_value.identifierSemanticValue.identifierName,
                                SYMBOL_UNDECLARED);
        functionCallNode->nodeType = ERROR_TYPE;
        return;
    }
    else if (function->attribute->attributeKind != FUNCTION_SIGNATURE) {
        printErrorMsgSpecial(functionCallNode, functionName->semantic_value.identifierSemanticValue.identifierName,
                                NOT_FUNCTION_NAME);
        functionCallNode->nodeType = ERROR_TYPE;
        return;
    }
    else {
        checkParameterPassing(paramList, function->attribute->attr.functionSignature->parameterList, function->name);
        functionCallNode->dataType = function->attribute->attr.functionSignature->returnType;        // fixed
        //fprintf(stderr, "mark %d\n", function->attribute->attr.functionSignature->returnType);
    }
}
// ----------   6 check statment    END --------------


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
            if(translatedType == SCALAR_TYPE_DESCRIPTOR && expectParam->type->kind == ARRAY_TYPE_DESCRIPTOR) {
                printErrorMsgSpecial2(param->linenumber, 
                                      param->semantic_value.identifierSemanticValue.identifierName, 
                                      expectParam->parameterName, 
                                      PASS_SCALAR_TO_ARRAY);
            }
            else if(translatedType == ARRAY_TYPE_DESCRIPTOR && expectParam->type->kind == SCALAR_TYPE_DESCRIPTOR) {
                printErrorMsgSpecial2(param->linenumber,
                                      param->semantic_value.identifierSemanticValue.identifierName,
                                      expectParam->parameterName, 
                                      PASS_ARRAY_TO_SCALAR);
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

// Maybe we don't need the following 2 functions ? 
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}

/*
 * The subtree is data, or unary/binary operation of data.
 * We need to check whether the data type is consistent
 *      1. no void type
 *      2. no string type
 *      3. no pointer type
 * We should propagate error type.
 */
void processExprNode(AST_NODE* exprNode)
{
    DATA_TYPE dataType1, dataType2;
    int type = exprNode->semantic_value.exprSemanticValue.kind;
    switch (exprNode->semantic_value.exprSemanticValue.kind) {
        case BINARY_OPERATION:
            dataType1 = processRelopExpr(exprNode->child);
            dataType2 = processRelopExpr(exprNode->child->rightSibling);
            if (dataType1 == VOID_TYPE || dataType2 == VOID_TYPE) {                     // void
                printErrorMsg(exprNode, VOID_OPERATION);
                exprNode->dataType = ERROR_TYPE;
                return;
            }
            if (dataType1 == CONST_STRING_TYPE || dataType2 == CONST_STRING_TYPE ) {    // constant string
                printErrorMsg(exprNode, STRING_OPERATION);
                exprNode->dataType = ERROR_TYPE;
                return;
            }
            if (dataType1 == INT_PTR_TYPE || dataType1 == FLOAT_PTR_TYPE 
                        || dataType2 == INT_PTR_TYPE || dataType2 == FLOAT_PTR_TYPE) {  // pointer
                printErrorMsg(exprNode, INCOMPATIBLE_ARRAY_DIMENSION);
                exprNode->dataType = ERROR_TYPE;
                return;
            }
            if (dataType1 == ERROR_TYPE || dataType1 == NONE_TYPE                       // error, none
                        || dataType2 == ERROR_TYPE || dataType2 == NONE_TYPE) {
                exprNode->dataType = ERROR_TYPE;
                return;
            }

            exprNode->dataType = getBiggerType(dataType1, dataType2);                   // all is well
            return;
        case UNARY_OPERATION:
            /**
             * there are only two kinds of unary operation : "!" and "-"
             * we allowed "!" on string constant and it will return INT_TYPE    <--   I discard this implemntation due to not meaningful enough.
             */
            dataType1 = processRelopExpr(exprNode->child);
            switch (dataType1) {
                case VOID_TYPE:                                                         // void
                    printErrorMsg(exprNode, VOID_OPERATION);
                    exprNode->dataType = ERROR_TYPE;
                    return;
                case CONST_STRING_TYPE:                                                 // constant string
                    printErrorMsg(exprNode, STRING_OPERATION);
                    exprNode->dataType = ERROR_TYPE;
                    return;
                case INT_PTR_TYPE:                                                      // pointer
                case FLOAT_PTR_TYPE:
                    printErrorMsg(exprNode, INCOMPATIBLE_ARRAY_DIMENSION);
                    exprNode->dataType = ERROR_TYPE;
                    return;
                case INT_TYPE:                                                          // int
                    exprNode->dataType = INT_TYPE;
                    break;
                case FLOAT_TYPE:                                                        // float
                    exprNode->dataType = FLOAT_TYPE;
                    break;
                default:                                                                // error
                    exprNode->dataType = ERROR_TYPE;
                    break;
            }
            break;
        default:
            fprintf(stderr, "[PARSER ERROR] no such type of expr\n");
            exit(255);
    }
}

/**
 *  The following 2 functions handle the id in the LHS and RHS respectively, roughly the same.
 *      1. Check whether it is declared before.
 *      2.  (a) if it is function pointer, print error
 *          (b) if it is a type, print error             // added
 *          (c) otherwise if it is scalar, return it
 *          (d) otherwise we check if it is an array slice
 *  Roughly Speaking
 *      processVariableLValue <------- processRelopExpr
 *      processRelopExpr:
 *          processConstValueNode(variable);
 *          processExprNode(variable);
 *          processVariableRValue(variable);
 *          checkFunctionCall(variable);
 *  The main difference between them is how to deal with the pointer.
 */
DATA_TYPE processVariableLValue(AST_NODE* idNode)
{
    SymbolTableEntry *symbol = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, 
                                /*onlyInCurrentScope*/false);
    if (symbol == NULL) {
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName,
                            SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return ERROR_TYPE;
    }



    if (symbol->attribute->attributeKind == FUNCTION_SIGNATURE) {
        printErrorMsgSpecial(idNode, symbol->name, IS_FUNCTION_NOT_VARIABLE);
        idNode->dataType = ERROR_TYPE;
        return ERROR_TYPE;
    }
    else if (symbol->attribute->attributeKind == TYPE_ATTRIBUTE){
        printErrorMsgSpecial(idNode, symbol->name, IS_TYPE_NOT_VARIABLE);
        idNode->dataType = ERROR_TYPE;
        return ERROR_TYPE;
    }
    else if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        /* actually, we should check whether idnode use it as an array, but too lazy */
        idNode->dataType = symbol->attribute->attr.typeDescriptor->properties.dataType;
    }
    else {
        int indexCount = 0;
        int maxIndexCount = symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
        DATA_TYPE indexType;
        AST_NODE* index = idNode->child;
        while (index != NULL) {
            ++indexCount;
            indexType = processRelopExpr(index);
            if (indexType != INT_TYPE) {
                printErrorMsg(index, ARRAY_SUBSCRIPT_NOT_INT);
            }
            index = index->rightSibling;
        }
        if (indexCount < maxIndexCount) {
            // not allowed pointer in the LHS of assign, since we don't know how to handle
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
            idNode->dataType = ERROR_TYPE;
            return ERROR_TYPE;
        }
        else if (indexCount == maxIndexCount) {
            idNode->dataType = symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;// fixed
        }
        else {
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
            idNode->dataType = ERROR_TYPE;
            return ERROR_TYPE;
        }
    }
    return idNode->dataType;
}

void processVariableRValue(AST_NODE* idNode)
{
    SymbolTableEntry *symbol = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, 
                                /*onlyInCurrentScope*/false);
    if (symbol == NULL) {
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName,
                            SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return;
    }
 

    if (symbol->attribute->attributeKind == FUNCTION_SIGNATURE) {
        printErrorMsgSpecial(idNode, symbol->name, IS_FUNCTION_NOT_VARIABLE);
        idNode->dataType = ERROR_TYPE;
        return;
    }
    else if (symbol->attribute->attributeKind == TYPE_ATTRIBUTE){
        printErrorMsgSpecial(idNode, symbol->name, IS_TYPE_NOT_VARIABLE);
        idNode->dataType = ERROR_TYPE;
        return;
    }
    else if (symbol->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        // check whether this node is referenced as array
        if (idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
            printErrorMsgSpecial(idNode, symbol->name, NOT_ARRAY);
            idNode->dataType = ERROR_TYPE;
        }
        else {
            idNode->dataType = symbol->attribute->attr.typeDescriptor->properties.dataType;
        }
    }
    else {
        int indexCount = 0;
        int maxIndexCount = symbol->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
        DATA_TYPE indexType;
        AST_NODE* index = idNode->child;
        while (index != NULL) {
            ++indexCount;
            indexType = processRelopExpr(index);
            if (indexType != INT_TYPE) {
                printErrorMsg(index, ARRAY_SUBSCRIPT_NOT_INT);
            }
            index = index->rightSibling;
        }
        if (indexCount < maxIndexCount) {
            if (symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType == INT_TYPE){  // fixed
                idNode->dataType = INT_PTR_TYPE;
            }else{
                idNode->dataType = FLOAT_PTR_TYPE;
            }
        }
        else if (indexCount == maxIndexCount) {
            idNode->dataType = symbol->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;// fixed
        }
        else {
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
            idNode->dataType = ERROR_TYPE;
            return;
        }
    }
}




void processConstValueNode(AST_NODE* constValueNode)
{
    // guarantee constValueNode->nodeType == CONST_VALUE_NODE
    switch (constValueNode->semantic_value.const1->const_type) {
        case INTEGERC:
            constValueNode->dataType = INT_TYPE;
            return;
        case FLOATC:
            constValueNode->dataType = FLOAT_TYPE;
            return;
        case STRINGC:
            constValueNode->dataType = CONST_STRING_TYPE;
            return;
        default:
            fprintf(stderr, "[PARSER ERROR] no such constant type\n");
            constValueNode->dataType = ERROR_TYPE;
            exit(255);
    }
}

// what's this ? previous homework with read/write ? 
void checkWriteFunction(AST_NODE* functionCallNode)
{
}

/*
 * In parser.y,     block : decl_list stmt_list 
 */
void processBlockNode(AST_NODE* blockNode)
{
    openScope();
    AST_NODE *nodeptr, *tmp;
    for(nodeptr = blockNode->child; nodeptr != NULL; nodeptr = nodeptr -> rightSibling){// 2 loops at most actually
        switch(nodeptr->nodeType) {
            case VARIABLE_DECL_LIST_NODE:                           // decl_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {
                    processDeclarationNode(tmp);
                }
                break;
            case STMT_LIST_NODE:                                    // stmt_list
                for (tmp = nodeptr->child; tmp != NULL; tmp = tmp->rightSibling) {
                    processStmtNode(tmp);
                }
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] void processBlockNode(AST_NODE* blockNode).\n");
                blockNode->dataType = ERROR_TYPE;
                exit(255);
        }
    }
    closeScope();
}

/*
 * Due to the overlapping, only need to divide the work to other functions.
 * In parser.y, there are 10 kinds of stmtNode, (2 return, 2 if) --> 8 types.
 */
void processStmtNode(AST_NODE* stmtNode)
{
    if(stmtNode->nodeType == BLOCK_NODE){
        processBlockNode(stmtNode);
    }else if(stmtNode->nodeType == NUL_NODE){
        return;
    }else if(stmtNode->nodeType == STMT_NODE){
        STMT_KIND kind = stmtNode->semantic_value.stmtSemanticValue.kind;
        switch(kind){
            case WHILE_STMT:
                checkWhileStmt(stmtNode);
                break;
            case FOR_STMT:
                checkForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                checkAssignmentStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                checkFunctionCall(stmtNode);
                break;
            case IF_STMT:
                checkIfStmt(stmtNode);
                break;
            case RETURN_STMT:
                checkReturnStmt(stmtNode);
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

/* 
 *  Put something here  ? ? 
 */
void processGeneralNode(AST_NODE *node)
{
}

/*
 * used by jason ?
 */
void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}



