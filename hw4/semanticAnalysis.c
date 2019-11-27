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
void processNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, bool ignoreFirstDimensionOfArray);
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
void processExprRelatedNode(AST_NODE* exprRelatedNode);
void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
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
    /*
    switch(errorMsgKind)
    {
    default:
        printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
        break;
    }
    */
}


void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind) {
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    switch (errorMsgKind) {
        case SYMBOL_IS_NOT_TYPE:
            printf("variable type should not be a variable\n");
            break;
        case SYMBOL_UNDECLARED:
            printf("symbol undeclared\n"); 
            break;
        case ARRAY_SIZE_MISSING:
            printf("array size missing\n");
            break;
        case ARRAY_SIZE_NOT_INT:
            printf("array size should be int\n");
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void semanticAnalysis(AST_NODE *root)
{
    processNode(root);
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
 * check if this is 
 * 1. int
 * 2. float
 * 3. void
 */
DATA_TYPE checkType (const char name[]) {
    typeDef* tmp = typeInt;
    while (tmp != NULL) {
        if (!strcmp(tmp->name, name))
            return INT_TYPE;
        tmp = tmp->next;
    }
    
    tmp = typeFloat;
    while (tmp != NULL) {
        if (!strcmp(tmp->name, name))
            return FLOAT_TYPE;
        tmp = tmp->next;
    }

    tmp = typeVoid;
    while (tmp != NULL) {
        if (!strcmp(tmp->name, name))
            return VOID_TYPE;
        tmp = tmp->next;
    }

    fprintf(stderr, "type %s not found\n", name);
    return -1;
}



void processNode(AST_NODE *programNode) {
    AST_NODE* start = programNode->child;
    AST_NODE* tmp;
    
    // first open a scope for global declaration
    openScope();

    for (start = programNode->child; start != NULL; start = start->rightSibling) {
        switch (start->nodeType) {
            case PROGRAM_NODE:
                fprintf(stderr, "[PARSER ERROR] multiple program node found\n");
                exit(255);
            case DECLARATION_NODE:
                processDeclarationNode(start);
                break;
            case IDENTIFIER_NODE:
            case PARAM_LIST_NODE:
            case NUL_NODE:
                break;
            case BLOCK_NODE:
                if (start->child != NULL) {
                    ++symbolTable.currentLevel;
                    processNode(start->child);
                    --symbolTable.currentLevel;
                }
                break;
            case VARIABLE_DECL_LIST_NODE:
                for (tmp = start->child; tmp != NULL; tmp = tmp->rightSibling) {
                    processDeclarationNode(tmp);
                }
                break;
            case STMT_LIST_NODE:
            case STMT_NODE:
            case EXPR_NODE:
            case CONST_VALUE_NODE:
            case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
            case NONEMPTY_RELOP_EXPR_LIST_NODE:
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
            declareIdList(declarationNode, false);
        case TYPE_DECL:
            break;
        case FUNCTION_DECL:
            break;
        case FUNCTION_PARAMETER_DECL:
            declareIdList(declarationNode, true);
            break;
        default:
            fprintf(stderr, "no such DECL kind\n");
            exit(255);

    }
}


void processTypeNode(AST_NODE* idNodeAsType) {
}

/**
 * process Declare Identifier List
 * this is either under
 *  1. VARIABLE_DECL
 *  2. FUNCTION_PARAMETER_DECL
 * the only difference is in FUNCTION_PARAMETER_DECL, we can ignore first dimension of array definition
 */
void declareIdList(AST_NODE* declarationNode, bool ignoreFirstDimensionOfArray) {
    int idx = 0;
    DATA_TYPE type;
    AST_NODE* dataType = declarationNode->child;
    
    /**
     * get declared type
     * this type name should not be a variable name
     */
    if (retrieveSymbol(dataType->semantic_value.identifierSemanticValue.identifierName) != NULL) {
        printErrorMsg(dataType, SYMBOL_IS_NOT_TYPE);
        return;
    }
    type = checkType(dataType->semantic_value.identifierSemanticValue.identifierName);
    if (type == -1) {
        printErrorMsg(declarationNode, SYMBOL_UNDECLARED);
    }

    for (AST_NODE* variable = dataType->rightSibling; variable != NULL; variable = variable->rightSibling) {
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
                break;
            default:
                fprintf(stderr, "[PARSER ERROR] no such kind of IdentifierSemanticValue\n");
        } // switch, variable type

    /**
     * now symbolTableEntry is ready
     * we need to add it into symbolTable
     */
    addSymbol(symbol);

    } // for, over variable
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
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

void checkFunctionCall(AST_NODE* functionCallNode)
{
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
}


void processExprRelatedNode(AST_NODE* exprRelatedNode)
{
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


void declareFunction(AST_NODE* declarationNode)
{
}
