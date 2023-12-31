#include "../inc/ast_funcs.h"
#include "../inc/code_gen.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/type_check.h"
#include "../inc/utils.h"
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("\nSee `%s --help`\n\n", argv[0]);
        print_error(ERR_ARGS, "Invalid number of arguments");
    }

    int out_file_idx = -1;
    int in_file_idx = -1;
    TargetFormat out_fmt = TARGET_FMT_DEFAULT;
    TargetCallingConvention call_conv = TARGET_CALL_CONV_WIN;
    TargetAssemblyDialect dialect = TARGET_ASM_DIALECT_DEFAULT;
    int is_verbose = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("%s", USAGE_STRING);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 ||
                   strcmp(argv[i], "--input") == 0) {
            i = i + 1;
            if (i > argc) {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS, "Expected Input file path after %s",
                            argv[i - 1]);
            }
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid input file path got another "
                            "possible command line option : `%s`",
                            argv[i]);
            }
            in_file_idx = i;
        } else if (strcmp(argv[i], "-o") == 0 ||
                   strcmp(argv[i], "--output") == 0) {
            i = i + 1;
            if (i > argc) {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS, "Expected Output file path after %s",
                            argv[i - 1]);
            }
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid file path got another possible "
                            "command line option : `%s`",
                            argv[i]);
            }
            out_file_idx = i;
        } else if (strcmp(argv[i], "-f") == 0 ||
                   strcmp(argv[i], "--format") == 0) {
            i = i + 1;
            if (i > argc) {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS, "Expected Output format after : `%s`",
                            argv[i - 1]);
            }
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid output format got another possible "
                            "command line option : `%s`",
                            argv[i]);
            }
            if (strcmp(argv[i], "default") == 0)
                out_fmt = TARGET_FMT_DEFAULT;
            else if (strcmp(argv[i], "x86_64-gnu-as") == 0)
                out_fmt = TARGET_FMT_X86_64_GNU_AS;
            else {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid output format, got : `%s`",
                            argv[i]);
            }
        } else if (strcmp(argv[i], "-cc") == 0 ||
                   strcmp(argv[i], "--call-conv") == 0) {
            i = i + 1;
            if (i > argc) {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS, "Expected Output format after : `%s`",
                            argv[i - 1]);
            }
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(
                    ERR_ARGS,
                    "Expected valid calling convention got another possible "
                    "command line option : `%s`",
                    argv[i]);
            }
            if (strcmp(argv[i], "default") == 0)
                call_conv = TARGET_CALL_CONV_WIN;
            else if (strcmp(argv[i], "linux") == 0)
                call_conv = TARGET_CALL_CONV_LINUX;
            else if (strcmp(argv[i], "windows") == 0)
                call_conv = TARGET_CALL_CONV_WIN;
            else {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid calling convention, got : `%s`",
                            argv[i]);
            }
        } else if (strcmp(argv[i], "-V") == 0 ||
                   strcmp(argv[i], "--verbose") == 0) {
            is_verbose = 1;
        } else if (strcmp(argv[i], "-v") == 0 ||
                   strcmp(argv[i], "--version") == 0) {
            printf("%s", VERSION_STRING);
            return 0;
        } else if (strcmp(argv[i], "-ad") == 0 ||
                   strcmp(argv[i], "--asm-dialect") == 0) {
            i = i + 1;
            if (i > argc) {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS, "Expected Output format after : `%s`",
                            argv[i - 1]);
            }
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid output format got another possible "
                            "command line option : `%s`",
                            argv[i]);
            }
            if (strcmp(argv[i], "default") == 0)
                dialect = TARGET_ASM_DIALECT_DEFAULT;
            else if (strcmp(argv[i], "att") == 0)
                dialect = TARGET_ASM_DIALECT_ATT;
            else if (strcmp(argv[i], "intel") == 0)
                dialect = TARGET_ASM_DIALECT_INTEL;
            else {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid output format, got : `%s`",
                            argv[i]);
            }
        } else {
            if (*argv[i] == '-') {
                printf("\nSee `%s --help`\n\n", argv[0]);
                print_error(ERR_ARGS,
                            "Expected valid argument got another possible "
                            "command line option : `%s`",
                            argv[i]);
            }
            if (in_file_idx != -1)
                print_warning(ERR_ARGS,
                              "Got multiple source files,using latest : `%s`",
                              argv[i]);
            in_file_idx = i;
        }
    }

    // Allocate the AST Node for the whole program.
    AstNode *program = node_alloc();
    program->type = TYPE_PROGRAM;

    ParsingContext *curr_context = create_default_parsing_context();

    // Open the file, lex and parse the file.
    if (in_file_idx == -1)
        print_error(ERR_ARGS, "Expected valid input file path");

    lex_and_parse(argv[in_file_idx], &curr_context, &program);

    if (is_verbose == 1) {
        print_ast_node(program, 0);
        putchar('\n');
        print_parsing_context(curr_context, 0);
    }

    // Type check the program.
    type_check_prog(curr_context, program);

    // Start Code generation.
    if (is_verbose == 1)
        printf("\n[+]CODE GENERATION BEGIN...\n");

    // If the output file name is not passed, use the input
    // file name as the base name for the assembly file.
    char *file_name = NULL;
    if (out_file_idx == -1) {
        file_name = strrchr(argv[in_file_idx], '/');
        file_name += 1; // Skip the `/`.
        int file_name_len = strlen(file_name);

        // TODO: Input file extensions will also need to
        // checked some time in the future to enforce proper usage.

        // Remove the last character of the extension, which
        // should be `.sy` for sypher files.
        file_name[file_name_len - 1] = '\0';

    } else
        file_name = argv[out_file_idx];

    target_codegen(curr_context, program, file_name, out_fmt, dialect,
                   call_conv);

    if (is_verbose == 1)
        printf("[+]CODE GENERATION COMPLETE\n");

    free_node(program);

    return 0;
}
