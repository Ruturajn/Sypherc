#include "../inc/ast_funcs.h"
#include "../inc/helpers.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"

char node_buf[NODE_BUF_SIZE] = {0};

char *get_node_str(AstNode *node) {
    switch (node->type) {
    case TYPE_NULL:
        snprintf(node_buf, NODE_BUF_SIZE, "NULL");
        break;
    case TYPE_PROGRAM:
        snprintf(node_buf, NODE_BUF_SIZE, "PROGRAM");
        break;
    case TYPE_ROOT:
        snprintf(node_buf, NODE_BUF_SIZE, "ROOT");
        break;
    case TYPE_INT:
        snprintf(node_buf, NODE_BUF_SIZE, "INT : %ld", node->ast_val.val);
        break;
    case TYPE_BINARY_OPERATOR:
        snprintf(node_buf, NODE_BUF_SIZE, "BINARY OP : %s", node->ast_val.node_symbol);
        break;
    case TYPE_VAR_DECLARATION:
        snprintf(node_buf, NODE_BUF_SIZE, "VAR DECL");
        break;
    case TYPE_VAR_ACCESS:
        snprintf(node_buf, NODE_BUF_SIZE, "VAR ACCESS : %s", node->ast_val.node_symbol);
        break;
    case TYPE_SYM:
        snprintf(node_buf, NODE_BUF_SIZE, "SYM : %s", node->ast_val.node_symbol);
        break;
    case TYPE_VAR_REASSIGNMENT:
        snprintf(node_buf, NODE_BUF_SIZE, "VAR REASSIGNMENT");
        break;
    case TYPE_FUNCTION:
        snprintf(node_buf, NODE_BUF_SIZE, "FUNCTION");
        break;
    case TYPE_FUNCTION_CALL:
        snprintf(node_buf, NODE_BUF_SIZE, "FUNCTION CALL");
        break;
    case TYPE_IF_CONDITION:
        snprintf(node_buf, NODE_BUF_SIZE, "IF CONDITION");
        break;
    case TYPE_POINTER:
        snprintf(node_buf, NODE_BUF_SIZE, "POINTER");
        break;
    case TYPE_ADDROF:
        snprintf(node_buf, NODE_BUF_SIZE, "ADDRESS OF");
        break;
    case TYPE_DEREFERENCE:
        snprintf(node_buf, NODE_BUF_SIZE, "DEREFERENCE");
        break;
    default:
        snprintf(node_buf, NODE_BUF_SIZE, "Unknown TYPE");
        break;
    }
    return node_buf;
}

void print_ast_node(AstNode *root_node, int indent) {
    if (root_node == NULL) {
        return;
    }
    for (int i = 0; i < indent; i++)
        putchar(' ');
    printf("%s\n", get_node_str(root_node));
    AstNode *child_node = root_node->child;
    while (child_node != NULL) {
        print_ast_node(child_node, indent + 4);
        child_node = child_node->next_child;
    }
}

int node_cmp(AstNode *node1, AstNode *node2) {
    if (node1 == NULL && node2 == NULL)
        return 1;
    if ((node1 == NULL && node2 != NULL) || (node1 != NULL && node2 == NULL))
        return 0;

    if ((node1->type == TYPE_VAR_ACCESS || node1->type == TYPE_SYM) &&
        (node2->type == TYPE_VAR_ACCESS || node2->type == TYPE_SYM)) {
        if (strcmp(node1->ast_val.node_symbol, node2->ast_val.node_symbol) == 0)
            return 1;
    }

    if (node1->type != node2->type) {
        return 0;
    }

    switch (node1->type) {
    case TYPE_NULL:
        break;
    case TYPE_PROGRAM:
        break;
    case TYPE_ROOT:
        printf("Compare Programs : Not implemented\n");
        break;
    case TYPE_INT:
        if (node1->ast_val.val == node2->ast_val.val)
            return 1;
        break;
    case TYPE_BINARY_OPERATOR:
        printf("TODO : BINARY OPERATOR!\n");
        break;
    case TYPE_VAR_DECLARATION:
        printf("TODO : VAR DECLARATION!\n");
        break;
    case TYPE_VAR_ACCESS:
        printf("TODO : VAR ACCESS!\n");
        break;
    case TYPE_SYM:
        if (node1->ast_val.node_symbol != NULL && node2->ast_val.node_symbol != NULL &&
            (strcmp(node1->ast_val.node_symbol, node2->ast_val.node_symbol) == 0))
            return 1;
        else if (node1->ast_val.node_symbol == NULL && node2->ast_val.node_symbol == NULL)
            return 1;
        break;
    case TYPE_VAR_REASSIGNMENT:
        printf("TODO : VAR REASSIGNMENT!\n");
        break;
    case TYPE_FUNCTION:
        printf("TODO : FUNCTION!\n");
        break;
    case TYPE_FUNCTION_CALL:
        printf("TODO : FUNCTION CALL!\n");
        break;
    case TYPE_IF_CONDITION:
        printf("TODO : IF CONDITION!\n");
        break;
    case TYPE_POINTER:
        printf("TODO : POINTER!\n");
        break;
    case TYPE_ADDROF:
        printf("TODO : ADDROF!\n");
        break;
    case TYPE_DEREFERENCE:
        printf("TODO : DEREFERENCE!\n");
        break;
    default:
        break;
    }

    return 0;
}

AstNode *node_alloc() {
    AstNode *new_node = (AstNode *)calloc(1, sizeof(AstNode));
    CHECK_NULL(new_node, "Unable to allocate memory for a new node", NULL);
    new_node->type = TYPE_NULL;
    new_node->child = NULL;
    new_node->next_child = NULL;
    new_node->ast_val.node_symbol = NULL;
    new_node->ast_val.val = 0;

    return new_node;
}

AstNode *create_node_symbol(char *symbol_str) {
    AstNode *sym_node = node_alloc();
    sym_node->type = TYPE_SYM;
    size_t len = strlen(symbol_str);
    sym_node->ast_val.node_symbol = (char *)calloc(len + 1, sizeof(char));
    strcpy(sym_node->ast_val.node_symbol, symbol_str);
    sym_node->ast_val.node_symbol[len] = '\0';
    return sym_node;
}

AstNode *create_node_none() {
    AstNode *null_node = node_alloc();
    null_node->type = TYPE_NULL;

    return null_node;
}

AstNode *create_node_int(long val) {
    AstNode *int_node = node_alloc();
    int_node->type = TYPE_INT;
    int_node->ast_val.val = val;

    return int_node;
}

void free_node(AstNode *node_to_free) {
    if (node_to_free == NULL) {
        return;
    }
    AstNode *temp = node_to_free->child;
    while (temp != NULL) {
        free_node(temp);
        temp = temp->child;
    }

    AstNode *temp1 = NULL;
    while (temp != NULL) {
        temp1 = temp->next_child;
        if (temp->ast_val.node_symbol != NULL)
            free(temp->ast_val.node_symbol);
        free(temp);
        temp = temp1;
    }
}

AstNode *node_symbol_from_token_create(LexedToken *token) {

    if (token == NULL) {
        return NULL;
    }

    AstNode *node = node_alloc();
    char *symbol_str = (char *)calloc(token->token_length + 1, sizeof(char));
    CHECK_NULL(symbol_str, "Unable to allocate memory for a new symbol node", NULL);
    memcpy(symbol_str, token->token_start, token->token_length);
    symbol_str[token->token_length] = '\0';
    node->ast_val.node_symbol = symbol_str;
    node->type = TYPE_SYM;
    return node;
}

int copy_node(AstNode *dst_node, AstNode *src_node) {
    if (src_node == NULL || dst_node == NULL)
        return 0;

    dst_node->type = src_node->type;

    if (src_node->ast_val.node_symbol != NULL)
        dst_node->ast_val.node_symbol = strdup(src_node->ast_val.node_symbol);
    else
        dst_node->ast_val.node_symbol = NULL;

    dst_node->ast_val.val = src_node->ast_val.val;

    AstNode *temp_child = src_node->child;
    AstNode *temp_dst_child = NULL;

    while (temp_child != NULL) {
        // Allocate memory for a new child node.
        AstNode *new_child = node_alloc();

        /**
         *  If temp_dst_child is NULL, it means we
         *  are copying the first child, in which case
         *  the dst_node's child is new_child, and we
         *  can set temp_dst_child to new_child.
         *  On the other hand, when temp_dst_child is not
         *  NULL, we can set it's next child to be the new_child,
         *  and move temp_dst_child forward, by setting it to
         *  its next child.
         */
        if (temp_dst_child != NULL)
            temp_dst_child->next_child = new_child;
        else
            dst_node->child = new_child;

        temp_dst_child = new_child;

        copy_node(temp_dst_child, temp_child);
        temp_child = temp_child->next_child;
    }
    return 0;
}

void add_ast_node_child(AstNode *parent_node, AstNode *child_to_add) {
    if (parent_node == NULL || child_to_add == NULL)
        return;
    if (parent_node->child == NULL) {
        parent_node->child = child_to_add;
        return;
    }
    AstNode *temp_child = parent_node->child;
    while (temp_child->next_child != NULL) {
        temp_child = temp_child->next_child;
    }
    temp_child->next_child = child_to_add;
}
