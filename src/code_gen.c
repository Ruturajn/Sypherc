#include "../inc/code_gen.h"
#include "../inc/ast_funcs.h"
#include "../inc/env_funcs.h"
#include "../inc/parser.h"
#include "../inc/utils.h"

char label_arr[LABEL_ARR_SIZE];
int label_cnt = 0;
int label_idx = 0;

char sym_arr[SYM_ARR_SIZE];
int sym_cnt = 0;
int sym_idx = 0;

char codegen_verbose = 1;

Reg *reg_create_new(char *reg_name) {
    Reg *new_reg = calloc(1, sizeof(Reg));
    CHECK_NULL(new_reg, "Unable to allocate memory for a new register", NULL);
    new_reg->reg_name = strdup(reg_name);
    new_reg->next_reg = NULL;
    return new_reg;
}

void reg_add_new(Reg **reg_head, char *reg_name) {
    if (*reg_head == NULL) {
        *reg_head = reg_create_new(reg_name);
        return;
    }

    Reg *temp = *reg_head;
    while (temp->next_reg != NULL) {
        temp = temp->next_reg;
    }
    temp->next_reg = reg_create_new(reg_name);
}

void reg_free(Reg *reg_head) {
    Reg *temp = NULL;
    while (temp != NULL) {
        temp = reg_head;
        reg_head = reg_head->next_reg;
        if (temp->reg_name != NULL)
            free(temp->reg_name);
        free(temp);
    }
}

RegDescriptor reg_alloc(Reg *reg_head) {
    Reg *temp = reg_head;
    RegDescriptor curr_reg = 0;
    while (temp != NULL) {
        if (!temp->reg_in_use) {
            temp->reg_in_use = 1;
            return curr_reg;
        }
        temp = temp->next_reg;
        curr_reg++;
    }
    print_error(ERR_MEM, "Unable to allocate memory for a new register", NULL, 0);
    return curr_reg;
}

char *get_reg_name(RegDescriptor reg_desc, Reg *reg_head) {
    Reg *temp = reg_head;
    RegDescriptor temp_desc = reg_desc;
    while (temp != NULL) {
        if (temp_desc == 0)
            return temp->reg_name;
        temp = temp->next_reg;
        temp_desc--;
    }
    print_error(ERR_COMMON, "Unable to find register name for register descriptor : `%d`", NULL,
                reg_desc);
    return NULL;
}

void reg_dealloc(Reg *reg_head, RegDescriptor reg_desc) {
    Reg *temp = reg_head;
    RegDescriptor temp_desc = reg_desc;
    while (temp != NULL) {
        if (temp_desc == 0) {
            temp->reg_in_use = 0;
            return;
        }
        temp = temp->next_reg;
        temp_desc--;
    }
    print_error(ERR_COMMON, "Unable to de-allocate register with register descriptor : `%d`", NULL,
                reg_desc);
}

char *gen_label() {
    char *label = label_arr + label_idx;
    label_idx += snprintf(label, LABEL_ARR_SIZE - label_idx, ".L%d", label_cnt);
    label_idx++;
    if (label_idx > LABEL_ARR_SIZE) {
        label_idx = 0;
        return gen_label();
    }
    label_cnt++;
    return label;
}

char *map_sym_to_addr_win(CGContext *cg_ctx, AstNode *sym_node) {
    // if (sym_node == NULL || sym_node->ast_val.node_symbol == NULL || cg_ctx == NULL)
    //     return NULL;
    char *sym = sym_arr + sym_idx;
    if (cg_ctx->parent_ctx == NULL)
        sym_idx +=
            snprintf(sym, SYM_ARR_SIZE - sym_idx, "%s(%%rip)", sym_node->ast_val.node_symbol);
    else {
        int stat = -1;
        AstNode *var_stack_offset = get_env(cg_ctx->local_env, sym_node, &stat);
        if (!stat)
            print_error(ERR_COMMON,
                        "Unable to get information from locals environment in "
                        "code gen context "
                        "for : `%s`",
                        sym_node->ast_val.node_symbol, 0);
        sym_idx +=
            snprintf(sym, SYM_ARR_SIZE - sym_idx, "%ld(%%rbp)", var_stack_offset->ast_val.val);
    }
    sym_idx++;
    if (sym_idx > LABEL_ARR_SIZE) {
        sym_idx = 0;
        sym = map_sym_to_addr_win(cg_ctx, sym_node);
    }
    sym_cnt++;
    return sym;
}

CGContext *create_codegen_context(CGContext *parent_ctx) {
    CGContext *new_ctx = calloc(1, sizeof(CGContext));
    CHECK_NULL(new_ctx, "Unable to allocate memory for new codegen context", NULL);
    new_ctx->parent_ctx = parent_ctx;
    new_ctx->local_env = create_env(NULL);
    new_ctx->local_offset = -32;
    return new_ctx;
}

void print_regs(Reg *reg_head) {
    Reg *temp = reg_head;
    while (temp != NULL) {
        printf("REG: %s, USE: %d\n", temp->reg_name, temp->reg_in_use);
        temp = temp->next_reg;
    }
}

void target_x86_64_win_codegen_expr(Reg *reg_head, ParsingContext *context,
                                    ParsingContext **ctx_next_child, AstNode *curr_expr,
                                    CGContext *cg_ctx, FILE *fptr_code) {
    switch (curr_expr->type) {
    case TYPE_VAR_DECLARATION:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Variable Declaration : `%s`\n",
                    curr_expr->child->ast_val.node_symbol);
        if (cg_ctx->parent_ctx == NULL)
            break;
        AstNode *var_node = NULL;
        int stat = -1;
        var_node = parser_get_var(context, curr_expr->child, &stat);
        if (stat == 0)
            print_error(ERR_COMMON, "Unable to find variable in environment : `%s`",
                        curr_expr->child->ast_val.node_symbol, 0);
        AstNode *type_node = NULL;
        type_node = parser_get_type(context, var_node, &stat);
        long size_in_bytes = type_node->child->ast_val.val;
        if (!stat)
            print_error(ERR_COMMON, "Couldn't find information for type : `%s`",
                        type_node->ast_val.node_symbol, 0);

        fprintf(fptr_code, "sub $%ld, %%rsp\n", size_in_bytes);
        cg_ctx->local_offset -= size_in_bytes;
        if (!set_env(&cg_ctx->local_env, curr_expr->child, create_node_int(cg_ctx->local_offset)))
            print_error(ERR_COMMON,
                        "Unable to set locals environment in code gen context "
                        "for : `%s`",
                        curr_expr->child->ast_val.node_symbol, 0);
        break;
    case TYPE_INT:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Literal Integer : %ld\n", curr_expr->ast_val.val);
        curr_expr->result_reg_desc = reg_alloc(reg_head);
        fprintf(fptr_code, "movq $%ld, %s\n", curr_expr->ast_val.val,
                get_reg_name(curr_expr->result_reg_desc, reg_head));
        break;
    case TYPE_VAR_ACCESS:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Variable Access : `%s`\n", curr_expr->ast_val.node_symbol);

        CGContext *var_cg_ctx = cg_ctx;
        stat = -1;
        AstNode *local_var_name;
        while (var_cg_ctx != NULL) {
            local_var_name =
                get_env_from_sym(var_cg_ctx->local_env, curr_expr->ast_val.node_symbol, &stat);
            if (stat)
                break;
            if (var_cg_ctx->parent_ctx != NULL)
                free_node(local_var_name);
            var_cg_ctx = var_cg_ctx->parent_ctx;
        }

        curr_expr->result_reg_desc = reg_alloc(reg_head);
        if (var_cg_ctx == NULL) {
            fprintf(fptr_code, "mov %s(%%rip), %s\n", curr_expr->ast_val.node_symbol,
                    get_reg_name(curr_expr->result_reg_desc, reg_head));
        } else {
            // Check if `stat` isn't 0, i.e. the variable was found.
            if (stat == 0)
                print_error(ERR_COMMON,
                            "Unable to find information regarding local "
                            "variable offset for: `%s`",
                            curr_expr->ast_val.node_symbol, 0);
            fprintf(fptr_code, "mov %ld(%%rbp), %s\n", local_var_name->ast_val.val,
                    get_reg_name(curr_expr->result_reg_desc, reg_head));
        }
        break;
    case TYPE_BINARY_OPERATOR:;
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Binary Operator : %s\n", curr_expr->ast_val.node_symbol);
        // Move the integers on the left and right hand side into different
        // registers.
        // See: https://www.felixcloutier.com/x86/
        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, curr_expr->child, cg_ctx,
                                       fptr_code);
        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child,
                                       curr_expr->child->next_child, cg_ctx, fptr_code);

        // Get the names of those registers, which contain the LHS and RHS
        // integers.
        char *reg_lhs = get_reg_name(curr_expr->child->result_reg_desc, reg_head);
        char *reg_rhs = get_reg_name(curr_expr->child->next_child->result_reg_desc, reg_head);

        if (strcmp(curr_expr->ast_val.node_symbol, ">") == 0) {
            curr_expr->result_reg_desc = reg_alloc(reg_head);
            RegDescriptor true_reg = reg_alloc(reg_head);

            fprintf(fptr_code, "mov $0, %s\n", get_reg_name(curr_expr->result_reg_desc, reg_head));
            fprintf(fptr_code, "mov $1, %s\n", get_reg_name(true_reg, reg_head));
            fprintf(fptr_code, "cmp %s, %s\n", reg_rhs, reg_lhs);
            fprintf(fptr_code, "cmovg %s, %s\n", get_reg_name(true_reg, reg_head),
                    get_reg_name(curr_expr->result_reg_desc, reg_head));

            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);
            reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);
            reg_dealloc(reg_head, true_reg);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "<") == 0) {
            curr_expr->result_reg_desc = reg_alloc(reg_head);
            RegDescriptor true_reg = reg_alloc(reg_head);

            fprintf(fptr_code, "mov $0, %s\n", get_reg_name(curr_expr->result_reg_desc, reg_head));
            fprintf(fptr_code, "mov $1, %s\n", get_reg_name(true_reg, reg_head));
            fprintf(fptr_code, "cmp %s, %s\n", reg_rhs, reg_lhs);
            fprintf(fptr_code, "cmovl %s, %s\n", get_reg_name(true_reg, reg_head),
                    get_reg_name(curr_expr->result_reg_desc, reg_head));

            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);
            reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);
            reg_dealloc(reg_head, true_reg);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "==") == 0) {
            curr_expr->result_reg_desc = reg_alloc(reg_head);
            RegDescriptor true_reg = reg_alloc(reg_head);

            fprintf(fptr_code, "mov $0, %s\n", get_reg_name(curr_expr->result_reg_desc, reg_head));
            fprintf(fptr_code, "mov $1, %s\n", get_reg_name(true_reg, reg_head));
            fprintf(fptr_code, "cmp %s, %s\n", reg_lhs, reg_rhs);
            fprintf(fptr_code, "cmove %s, %s\n", get_reg_name(true_reg, reg_head),
                    get_reg_name(curr_expr->result_reg_desc, reg_head));

            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);
            reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);
            reg_dealloc(reg_head, true_reg);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "+") == 0) {
            curr_expr->result_reg_desc = curr_expr->child->next_child->result_reg_desc;
            // Add those registers and save the result in the RHS register.
            fprintf(fptr_code, "add %s, %s\n", reg_lhs, reg_rhs);

            // De-allocate the LHS register since it is not in use anymore.
            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "-") == 0) {
            curr_expr->result_reg_desc = curr_expr->child->result_reg_desc;
            // Subtract those registers and save the result in the LHS register.
            // `sub` operation subtracts the first operand from the second
            // operand, and stores it in the second operand.
            fprintf(fptr_code, "sub %s, %s\n", reg_rhs, reg_lhs);

            // De-allocate the LHS register since it is not in use anymore.
            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "<<") == 0) {
            curr_expr->result_reg_desc = curr_expr->child->result_reg_desc;
            // Since shift left is destructive, we use the expression result
            // register as the LHS register. The RHS or the amount by which the
            // shift left needs to be done is placed into RCX, which is used by
            // the SHL instruction by default and the final value is stored in
            // the LHS register.
            fprintf(fptr_code,
                    "push %%rcx\n"
                    "mov %s, %%rcx\n"
                    "sal %%cl, %s\n"
                    "pop %%rcx\n",
                    reg_rhs, reg_lhs);

            // De-allocate the RHS register since it is not in use anymore.
            reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);

        } else if (strcmp(curr_expr->ast_val.node_symbol, ">>") == 0) {
            curr_expr->result_reg_desc = curr_expr->child->result_reg_desc;
            // Since shift right is destructive, we use the expression result
            // register as the LHS register. The RHS or the amount by which the
            // shift right needs to be done is placed into RCX, which is used by
            // the SHL instruction by default and the final value is stored in
            // the LHS register.
            fprintf(fptr_code,
                    "push %%rcx\n"
                    "mov %s, %%rcx\n"
                    "sar %%cl, %s\n"
                    "pop %%rcx\n",
                    reg_rhs, reg_lhs);

            // De-allocate the RHS register since it is not in use anymore.
            reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "*") == 0) {
            curr_expr->result_reg_desc = curr_expr->child->next_child->result_reg_desc;
            fprintf(fptr_code, "imul %s, %s\n", reg_lhs, reg_rhs);

            // De-allocate the RHS register since it is not in use anymore.
            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);

        } else if (strcmp(curr_expr->ast_val.node_symbol, "/") == 0 ||
                   strcmp(curr_expr->ast_val.node_symbol, "%") == 0) {

            char is_modulus = 0;
            if (strcmp(curr_expr->ast_val.node_symbol, "%") == 0)
                is_modulus = 1;

            // Quotient is saved in RAX, and remainder is saved in RDX.
            // So there registers need to be saved.
            fprintf(fptr_code, "pushq %%rax\n"
                               "pushq %%rdx\n");

            // Move result into RAX only if it isn't the result register.
            if (strcmp(get_reg_name(curr_expr->child->result_reg_desc, reg_head), "%rax") != 0)
                fprintf(fptr_code, "mov %s, %%rax\n",
                        get_reg_name(curr_expr->child->result_reg_desc, reg_head));

            // Move the value from LHS into RAX. Use signed division instead of
            // unsigned to avoid division error, when using negative numbers.
            // Sign extend the value in RAX to RDX.
            // RDX is used as the 8 high-bytes of a 16 byte number
            // stored in RDX:RAX. Convert 8 bytes into a 16 byte number using
            // `cqto` (convert quad word to octo word).
            fprintf(fptr_code, "cqto\n");

            // Call `idiv` instruction with the RHS register.
            fprintf(fptr_code, "idiv %s\n",
                    get_reg_name(curr_expr->child->next_child->result_reg_desc, reg_head));

            // Move the result that is stored in RAX, into the expression's result register.
            curr_expr->result_reg_desc = reg_alloc(reg_head);
            if (is_modulus) {
                if (strcmp(get_reg_name(curr_expr->result_reg_desc, reg_head), "%rdx") != 0)
                    fprintf(fptr_code, "mov %%rdx, %s\n",
                            get_reg_name(curr_expr->result_reg_desc, reg_head));
            } else {
                if (strcmp(get_reg_name(curr_expr->result_reg_desc, reg_head), "%rax") != 0)
                    fprintf(fptr_code, "mov %%rax, %s\n",
                            get_reg_name(curr_expr->result_reg_desc, reg_head));
            }

            fprintf(fptr_code, "pop %%rdx\n"
                               "pop %%rax\n");

            // De-allocate the RHS/LHS register since they are not in use anymore.
            // reg_dealloc(reg_head, curr_expr->child->result_reg_desc);
            // reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);

        } else
            print_error(ERR_COMMON, "Found unknown binary operator : `%s`",
                        curr_expr->ast_val.node_symbol, 0);
        break;
    case TYPE_FUNCTION_CALL:;
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Function Call : `%s`\n", curr_expr->child->ast_val.node_symbol);

        // Save RAX because, it may not be preserved after the function call,
        // because the return value is always placed into RAX.
        fprintf(fptr_code, "pushq %%rax\n");

        AstNode *call_params = curr_expr->child->next_child->child;
        int param_count = 0;
        while (call_params != NULL) {
            target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, call_params, cg_ctx,
                                           fptr_code);
            fprintf(fptr_code, "pushq %s\n", get_reg_name(call_params->result_reg_desc, reg_head));
            reg_dealloc(reg_head, call_params->result_reg_desc);
            call_params = call_params->next_child;
            param_count++;
        }
        /**
         * Consider the following assembly code:
         * mov $3, %rax
         * pushq %rax
         * call <func_name>
         * add $8, %rsp
         *
         * When the `call` instruction is executeed, this is the state of the
         * stack
         * |           3           |
         * | call - return address |
         * |       Old  RBP        | <-- RSP
         * The `old RBP` comes into the picture because we push RBP onto the
         * stack whenever we define a function. So, now to get to 3, we will
         * travel up 8 bytes to reach the `call - return address`, and 8 more
         * to reach our argument, i.e. 3.
         */

        // Now that we treats functions as variables for calling them we need to
        // var access the name of the "function variable" and call it's result
        // register. See TYPE_FUNCTION for more details.

        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, curr_expr->child, cg_ctx,
                                       fptr_code);
        fprintf(fptr_code, "call *%s\n", get_reg_name(curr_expr->child->result_reg_desc, reg_head));
        reg_dealloc(reg_head, curr_expr->child->result_reg_desc);

        if (param_count > 0)
            fprintf(fptr_code, "add $%d, %%rsp\n", param_count * 8);

        curr_expr->result_reg_desc = reg_alloc(reg_head);
        if (strcmp(get_reg_name(curr_expr->result_reg_desc, reg_head), "%rax") != 0) {
            fprintf(fptr_code, "mov %%rax, %s\n",
                    get_reg_name(curr_expr->result_reg_desc, reg_head));

            // Restore the value for RAX from stack.
            fprintf(fptr_code, "pop %%rax\n");
        } else
            fprintf(fptr_code, "add $8, %%rsp\n");
        break;
    case TYPE_FUNCTION:;
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Function Definition\n");

        ParsingContext *tmp_ctx = context;
        AstNode *func_id = NULL;
        stat = -1;
        while (tmp_ctx != NULL) {
            func_id = get_env_from_val(context->funcs, curr_expr, &stat);
            if (stat)
                break;
            tmp_ctx = tmp_ctx->parent_ctx;
            if (tmp_ctx != NULL)
                free_node(func_id);
        }
        char *func_name = NULL;
        if (stat)
            func_name = func_id->ast_val.node_symbol;
        else
            func_name = gen_label();

        target_x86_64_win_codegen_func(reg_head, cg_ctx, context, ctx_next_child, func_name,
                                       curr_expr, fptr_code);

        /**
         * Now that functions are being treated as variables we can use
         * the following assembly to re-assign them to a different function
         * signature and use that to call the actual function.
         *
         *      jmp after_foo_label
         * foo_label:
         *      push %rbp
         *      mov %rsp, %rbp
         *      sub $32, %rsp
         *      mov $69, %rax
         *      add $32, %rsp
         * after_foo_label:
         *      lea foo_label(%rip), %rax
         *      mov %rax, bar(%rip)
         *      mov bar(%rip), %rax
         *      call *%rax
         */

        curr_expr->result_reg_desc = reg_alloc(reg_head);
        fprintf(fptr_code, "lea %s(%%rip), %s\n", func_name,
                get_reg_name(curr_expr->result_reg_desc, reg_head));
        break;
    case TYPE_VAR_REASSIGNMENT:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Variable Re-assignment\n");
        AstNode *temp_sym = curr_expr->child;
        while (temp_sym != NULL && temp_sym->type != TYPE_VAR_ACCESS)
            temp_sym = temp_sym->child;
        if (temp_sym == NULL)
            print_error(ERR_COMMON,
                        "Unable to find valid variable access in a variable Re-assignment", NULL,
                        0);
        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child,
                                       curr_expr->child->next_child, cg_ctx, fptr_code);

        char *res_string = NULL;
        char res_string_free = 0;
        if (curr_expr->child->type == TYPE_VAR_ACCESS) {
            res_string = map_sym_to_addr_win(cg_ctx, temp_sym);
        } else {
            res_string_free = 1;
            target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, curr_expr->child,
                                           cg_ctx, fptr_code);
            res_string = get_reg_name(curr_expr->child->result_reg_desc, reg_head);
            char *res_string_copy = strdup(res_string);
            size_t total_len = strlen(res_string) + 3;
            res_string = calloc(total_len, sizeof(char));
            snprintf(res_string, total_len, "(%s)", res_string_copy);
            res_string[total_len - 1] = '\0';
            free(res_string_copy);
        }

        fprintf(fptr_code, "mov %s, %s\n",
                get_reg_name(curr_expr->child->next_child->result_reg_desc, reg_head), res_string);
        // De-allocate RHS result register.
        reg_dealloc(reg_head, curr_expr->child->next_child->result_reg_desc);

        if (res_string_free) {
            free(res_string);
            // De-allocate LHS result register.
            reg_dealloc(reg_head, curr_expr->child->result_reg_desc);
        }

        break;
    case TYPE_IF_CONDITION:;
        if (codegen_verbose)
            fprintf(fptr_code, ";#; IF Block\n");
        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, curr_expr->child, cg_ctx,
                                       fptr_code);

        if (codegen_verbose)
            fprintf(fptr_code, ";#; If Condition\n");

        char *res_reg = get_reg_name(curr_expr->child->result_reg_desc, reg_head);
        char *else_label = gen_label();
        char *after_else_label = gen_label();
        fprintf(fptr_code, "test %s, %s\n", res_reg, res_reg);
        fprintf(fptr_code, "jz %s\n", else_label);
        reg_dealloc(reg_head, curr_expr->child->result_reg_desc);

        if (codegen_verbose)
            fprintf(fptr_code, ";#; If Then Body\n");

        ParsingContext *ctx = context;
        ParsingContext *ctx_child = *ctx_next_child;
        if (*ctx_next_child != NULL) {
            ctx = *ctx_next_child;
            ctx_child = (*ctx_next_child)->child;
            *ctx_next_child = (*ctx_next_child)->next_child;
        }

        // The if body comes here.
        AstNode *last_expr = NULL;
        AstNode *if_expr = curr_expr->child->next_child->child;
        while (if_expr != NULL) {
            target_x86_64_win_codegen_expr(reg_head, ctx, &ctx_child, if_expr, cg_ctx, fptr_code);
            if (last_expr != NULL)
                reg_dealloc(reg_head, last_expr->result_reg_desc);
            last_expr = if_expr;
            if_expr = if_expr->next_child;
        }

        curr_expr->result_reg_desc = reg_alloc(reg_head);
        fprintf(fptr_code, "mov %s, %s\n", get_reg_name(last_expr->result_reg_desc, reg_head),
                get_reg_name(curr_expr->result_reg_desc, reg_head));
        reg_dealloc(reg_head, last_expr->result_reg_desc);
        fprintf(fptr_code, "jmp %s\n", after_else_label);

        if (codegen_verbose)
            fprintf(fptr_code, ";#; Else Body\n");
        fprintf(fptr_code, "%s:\n", else_label);

        // Else body
        last_expr = NULL;
        AstNode *else_expr = curr_expr->child->next_child->next_child;
        if (else_expr != NULL) {
            ctx = context;
            ctx_child = *ctx_next_child;
            if (*ctx_next_child != NULL) {
                ctx = *ctx_next_child;
                ctx_child = (*ctx_next_child)->child;
                *ctx_next_child = (*ctx_next_child)->next_child;
            }

            else_expr = else_expr->child;
            while (else_expr != NULL) {
                target_x86_64_win_codegen_expr(reg_head, ctx, &ctx_child, else_expr, cg_ctx,
                                               fptr_code);
                if (last_expr != NULL)
                    reg_dealloc(reg_head, last_expr->result_reg_desc);
                last_expr = else_expr;
                else_expr = else_expr->next_child;
            }

            fprintf(fptr_code, "mov %s, %s\n", get_reg_name(last_expr->result_reg_desc, reg_head),
                    get_reg_name(curr_expr->result_reg_desc, reg_head));
            reg_dealloc(reg_head, last_expr->result_reg_desc);
        } else {
            // If there is an 'if' statement with no else we need to set the
            // result register for the 'if' statement.
            fprintf(fptr_code, "mov $0, %s\n", get_reg_name(curr_expr->result_reg_desc, reg_head));
        }

        fprintf(fptr_code, "%s:\n", after_else_label);

        break;
    case TYPE_DEREFERENCE:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; Dereference\n");
        target_x86_64_win_codegen_expr(reg_head, context, ctx_next_child, curr_expr->child, cg_ctx,
                                       fptr_code);
        curr_expr->result_reg_desc = curr_expr->child->result_reg_desc;
        break;
    case TYPE_ADDROF:
        if (codegen_verbose)
            fprintf(fptr_code, ";#; AddressOf\n");
        curr_expr->result_reg_desc = reg_alloc(reg_head);
        fprintf(fptr_code, "lea %s, %s\n", map_sym_to_addr_win(cg_ctx, curr_expr->child),
                get_reg_name(curr_expr->result_reg_desc, reg_head));
        break;
    default:
        break;
    }
}

void target_x86_64_win_codegen_func(Reg *reg_head, CGContext *cg_ctx, ParsingContext *context,
                                    ParsingContext **ctx_next_child, char *func_name, AstNode *func,
                                    FILE *fptr_code) {
    cg_ctx = create_codegen_context(cg_ctx);

    /**
     * Storing the offset for parameters passed to the function
     * so that we know, which register to use for passing them
     * to the function.
     *
     * We also set the locals environment to have the name of the
     * parameter bound to an integer with the stack offset.
     * Example:
     * jmp afterfoo
     * foo:
     *     push %rbp
     *     mov %rsp, %rbp
     *
     *     pop %rbp
     *     ret
     * Due to the first and second instructions in the function, we
     * have moved 8 bytes twice in the stack, i.e. 16 bytes,
     * so we will need to account for them while binding the value to
     * the parameter names. Hence notice 'param_cnt' starts from '2',
     * to make space for the first and second instructions.
     */
    AstNode *func_param_list = func->child->next_child->child;
    long param_cnt = 1;
    while (func_param_list != NULL) {
        param_cnt++;
        if (!set_env(&(cg_ctx->local_env), func_param_list->child, create_node_int(param_cnt * 8)))
            print_error(ERR_COMMON,
                        "Unable to set locals environment in code gen context "
                        "for : `%s`",
                        func_param_list->child->ast_val.node_symbol, 0);
        func_param_list = func_param_list->next_child;
    }

    // Function protection
    fprintf(fptr_code, "jmp after%s\n", func_name);

    fprintf(fptr_code, "%s:\n", func_name);

    // Function header.
    fprintf(fptr_code, "%s", FUNC_HEADER_x86_64);

    fprintf(fptr_code, "push %%rbx\n"
                       "push %%rsi\n"
                       "push %%rdi\n");

    ParsingContext *ctx = context;
    ParsingContext *ctx_child = *ctx_next_child;
    if (*ctx_next_child != NULL) {
        ctx = *ctx_next_child;
        ctx_child = (*ctx_next_child)->child;
        *ctx_next_child = (*ctx_next_child)->next_child;
    }
    AstNode *temp_expr = func->child->next_child->next_child->child;
    AstNode *last_expr = NULL;
    while (temp_expr != NULL) {
        target_x86_64_win_codegen_expr(reg_head, ctx, &ctx_child, temp_expr, cg_ctx, fptr_code);
        reg_dealloc(reg_head, temp_expr->result_reg_desc);
        last_expr = temp_expr;
        temp_expr = temp_expr->next_child;
    }

    // Function footer.
    if (strcmp(get_reg_name(last_expr->result_reg_desc, reg_head), "%rax") != 0)
        fprintf(fptr_code, "mov %s, %%rax\n", get_reg_name(last_expr->result_reg_desc, reg_head));

    fprintf(fptr_code, "pop %%rbx\n"
                       "pop %%rsi\n"
                       "pop %%rdi\n");

    fprintf(fptr_code, "add $%ld, %%rsp\n", -cg_ctx->local_offset);
    fprintf(fptr_code, "%s", FUNC_FOOTER_x86_64);

    // Jump after function protection
    fprintf(fptr_code, "after%s:\n", func_name);
    cg_ctx = cg_ctx->parent_ctx;
}

void target_x86_64_win_codegen_prog(ParsingContext *context, AstNode *program, CGContext *cg_ctx,
                                    FILE *fptr_code) {

    Reg *reg_head = reg_create_new("%rax");
    reg_add_new(&reg_head, "%r10");
    reg_add_new(&reg_head, "%r11");
    reg_add_new(&reg_head, "%rbx");
    reg_add_new(&reg_head, "%rsi");
    reg_add_new(&reg_head, "%rdi");

    fprintf(fptr_code, ".section .data\n");

    IdentifierBind *temp_var_bind = context->vars->binding;
    AstNode *temp_var_type_id = NULL;
    int status = -1;
    while (temp_var_bind != NULL) {
        temp_var_type_id = node_alloc();
        *temp_var_type_id = *temp_var_bind->id_val;
        temp_var_type_id->child = NULL;
        temp_var_type_id->next_child = NULL;
        AstNode *var = parser_get_type(context, temp_var_type_id, &status);
        if (!status)
            print_error(ERR_COMMON, "Unable to retrieve value from environment for : `%s`",
                        temp_var_type_id->ast_val.node_symbol, 0);

        fprintf(fptr_code, "%s: .space %ld\n", temp_var_bind->identifier->ast_val.node_symbol,
                var->child->ast_val.val);

        temp_var_bind = temp_var_bind->next_id_bind;
        free_node(temp_var_type_id);
    }

    fprintf(fptr_code,
            ".section .text\n"
            ".global main\n"
            "main:\n"
            "%s",
            FUNC_HEADER_x86_64);

    ParsingContext *ctx_next_child = context->child;
    AstNode *curr_expr = program->child;
    AstNode *last_expr = NULL;
    while (curr_expr != NULL) {
        if (curr_expr->type == TYPE_NULL) {
            curr_expr = curr_expr->next_child;
            continue;
        }
        target_x86_64_win_codegen_expr(reg_head, context, &ctx_next_child, curr_expr, cg_ctx,
                                       fptr_code);
        reg_dealloc(reg_head, curr_expr->result_reg_desc);
        last_expr = curr_expr;
        curr_expr = curr_expr->next_child;
    }

    if (strcmp(get_reg_name(last_expr->result_reg_desc, reg_head), "%rax") != 0)
        fprintf(fptr_code, "mov %s, %%rax\n", get_reg_name(last_expr->result_reg_desc, reg_head));
    fprintf(fptr_code, "add $%ld, %%rsp\n", -cg_ctx->local_offset);
    fprintf(fptr_code, "%s", FUNC_FOOTER_x86_64);

    reg_free(reg_head);
}

void target_codegen(ParsingContext *context, AstNode *program, char *output_file_path,
                    TargetType type) {
    if (context == NULL || program == NULL)
        print_error(ERR_COMMON,
                    "NULL program node passed for code generation to "
                    "`target_codegen()`",
                    NULL, 0);

    FILE *fptr_code = NULL;
    if (output_file_path == NULL) {
        fptr_code = fopen("code_gen.s", "w");
        if (fptr_code == NULL)
            print_error(ERR_FILE_OPEN, "Unable to open default file for code generation : `%s`",
                        "code_gen.s", 0);
    } else {
        fptr_code = fopen(output_file_path, "w");
        if (fptr_code == NULL)
            print_error(ERR_FILE_OPEN, "Unable to open file for code generation : `%s`",
                        output_file_path, 0);
    }

    CGContext *cg_ctx = create_codegen_context(NULL);

    if (type == TARGET_DEFAULT || type == TARGET_x86_64_WIN)
        target_x86_64_win_codegen_prog(context, program, cg_ctx, fptr_code);

    fclose(fptr_code);
}
