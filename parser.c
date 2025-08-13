#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    Token *tokens;
    int count;
    int position;
} TokenArray;

TokenArray *tokenize(const char *input) {
    Lexer *lexer = lexer_init(input);
    TokenArray *array = malloc(sizeof(TokenArray));
    array->tokens = NULL;
    array->count = 0;
    array->position = 0;

    Token *token;
    do {
        token = lexer_next_token(lexer);
        array->tokens = realloc(array->tokens, (array->count + 1) * sizeof(Token));
        array->tokens[array->count] = *token;
        if (token->value) {
            array->tokens[array->count].value = strdup(token->value);
        }
        ++array->count;
        free_token(token);
    } while (array->tokens[array->count - 1].type != TOKEN_EOF);

    free_lexer(lexer);
    return array;
}

void free_token_array(TokenArray *array) {
    for (int i = 0; i < array->count; ++i) {
        if (array->tokens[i].value) free(array->tokens[i].value);
    }
    free(array->tokens);
    free(array);
}

Token *peek_token(TokenArray *array) {
    if (array->position >= array->count) return NULL;
    return &array->tokens[array->position];
}

Token *next_token(TokenArray *array) {
    if (array->position >= array->count) return NULL;
    return &array->tokens[array->position++];
}

typedef enum {
    NODE_COMMAND,
    NODE_PIPE,
    NODE_REDIRECTION,
    NODE_AND,
    NODE_OR,
    NODE_SEQUENCE,
    NODE_BACKGROUND,
    NODE_SUBSHELL
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode *left;
    struct ASTNode *right;
    char **args;
    char *input_file;
    char *output_file;
    int append;
} ASTNode;

ASTNode *create_node(NodeType type, ASTNode *left, ASTNode *right);
ASTNode *parse_command(TokenArray *array);
ASTNode *parse_pipeline(TokenArray *array);
ASTNode *parse_and_or(TokenArray *array);
ASTNode *parse_sequence_background(TokenArray *array);
ASTNode *parse_expression(TokenArray *array);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int level);
void test_parser(const char *input);

ASTNode *create_node(NodeType type, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->left = left;
    node->right = right;
    node->args = NULL;
    node->input_file = NULL;
    node->output_file = NULL;
    node->append = 0;
    return node;
}

ASTNode *parse_command(TokenArray *array) {
    Token *token = next_token(array);
    if (!token) return NULL;

    if (token->type == TOKEN_LPAREN) {
        ASTNode *subshell = parse_expression(array);
        token = next_token(array);
        if (!token || token->type != TOKEN_RPAREN) {
            fprintf(stderr, "Error: expected ')'\n");
            exit(EXIT_FAILURE);
        }
        return create_node(NODE_SUBSHELL, subshell, NULL);
    }

    if (token->type != TOKEN_COMMAND) {
        fprintf(stderr, "Error: expected command\n");
        exit(EXIT_FAILURE);
    }

    char **args = malloc(2 * sizeof(char *));
    args[0] = strdup(token->value);
    args[1] = NULL;
    int args_count = 1;

    ASTNode *command_node = create_node(NODE_COMMAND, NULL, NULL);
    command_node->args = args;

    while ((token = peek_token(array))) {
        if (token->type == TOKEN_INPUT_REDIR) {
            next_token(array);
            token = next_token(array);
            if (!token || token->type != TOKEN_COMMAND) {
                fprintf(stderr, "Error: expected filename after '<'\n");
                exit(EXIT_FAILURE);
            }
            command_node->input_file = strdup(token->value);
        }

        else if (token->type == TOKEN_OUTPUT_REDIR || token->type == TOKEN_APPEND_REDIR) {
            int is_append = (token->type == TOKEN_APPEND_REDIR);
            next_token(array);
            token = next_token(array);
            if (!token || token->type != TOKEN_COMMAND) {
                fprintf(stderr, "Error: expected filename after '%s'\n", is_append ? ">>" : ">");
                exit(EXIT_FAILURE);
            }
            command_node->output_file = strdup(token->value);
            command_node->append = is_append;
        }

        else if (token->type == TOKEN_COMMAND) {
            next_token(array);
            args_count++;
            args = realloc(args, (args_count + 1) * (sizeof(char *)));
            args[args_count - 1] = strdup(token->value);
            args[args_count] = NULL;
        }

        else break;
    }

    return command_node;
}

ASTNode *parse_pipeline(TokenArray *array) {
    ASTNode *left = parse_command(array);

    Token *token = peek_token(array);
    if (token && token->type == TOKEN_PIPE) {
        next_token(array);
        ASTNode *right = parse_pipeline(array);
        return create_node(NODE_PIPE, left, right);
    }

    return left;
}

ASTNode *parse_and_or(TokenArray *array) {
    ASTNode *left = parse_pipeline(array);

    Token *token = peek_token(array);
    if (token) {
        if (token->type == TOKEN_AND) {
            next_token(array);
            ASTNode *right = parse_and_or(array);
            return create_node(NODE_AND, left, right);
        }
        else if (token->type == TOKEN_OR) {
            next_token(array);
            ASTNode *right = parse_and_or(array);
            return create_node(NODE_OR, left, right);
        }
    }

    return left;
}

ASTNode *parse_sequence_background(TokenArray *array) {
    ASTNode *left = parse_and_or(array);

    Token *token = peek_token(array);
    if (token) {
        if (token->type == TOKEN_SEMICOLON) {
            next_token(array);
            ASTNode *right = parse_sequence_background(array);
            return create_node(NODE_SEQUENCE, left, right);
        }

        else if (token->type == TOKEN_BACKGROUND) {
            next_token(array);
            return create_node(NODE_BACKGROUND, left, NULL);
        }
    }

    return left;
}

ASTNode *parse_expression(TokenArray *array) {
    return parse_sequence_background(array);
}

void free_ast(ASTNode *node) {
    if (!node) return;

    free_ast(node->left);
    free_ast(node->right);

    if (node->args) {
        for (int i = 0; node->args[i]; ++i) {
            free(node->args[i]);
        }
        free(node->args);
    }

    if (node->input_file) free(node->input_file);
    if (node->output_file) free(node->output_file);

    free(node);
}

void print_ast(ASTNode *node, int level) {
    if (!node) return;

    for (int i = 0; i < level; ++i) printf("  ");

    switch (node->type) {
        case NODE_COMMAND:
            printf("COMMAND:");
            for (int i = 0; node->args && node->args[i]; ++i) {
                printf(" %s", node->args[i]);
            }
            if (node->input_file) printf(" < %s", node->input_file);
            if (node->output_file) printf(" %s %s", node->append ? ">>" : ">", node->output_file);
            break;
        case NODE_PIPE: printf("PIPE"); break;
        case NODE_AND: printf("AND"); break;
        case NODE_OR: printf("OR"); break;
        case NODE_SEQUENCE: printf("SEQUENCE"); break;
        case NODE_BACKGROUND: printf("BACKGROUND"); break;
        case NODE_SUBSHELL: printf("SUBSHELL"); break;
    }
    printf("\n");

    print_ast(node->left, level + 1);
    print_ast(node->right, level + 1);
}

void test_parser(const char *input) {
    printf("Parsing \"%s\"\n", input);
    TokenArray *tokens = tokenize(input);
    ASTNode *ast = parse_expression(tokens);
    print_ast(ast, 0);
    free_ast(ast);
    free_token_array(tokens);
    printf("\n");
}

int main() {
    test_parser("echo 'Hello, world'");
    test_parser("ls -l");
    test_parser("ls -l | grep \"test\" > output.txt && (cd dir; ls) &");
    test_parser("cat < input.txt | sort | uniq >> 'output file.txt' || echo \"Error\"");
    test_parser("(ps aux; ls -a) && pwd");
    return 0;
}