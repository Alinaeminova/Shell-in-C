#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

bool has_unclosed_quotes(const char *str) {
    bool in_single_quotes = false;
    bool in_double_quotes = false;
    bool escape_next = false;

    for (int i = 0; str[i]; ++i) {
        if (escape_next) {
            escape_next = false;
            continue;
        }
        if (str[i] == '\\') {
            escape_next = true;
            continue;
        }
        if (str[i] == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
        }
        else if (str[i] == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
        }
    }
    return in_single_quotes || in_double_quotes;
}

bool has_imbalanced_brackets(const char *str) {
    bool in_single_quotes = false;
    bool in_double_quotes = false;
    bool escape_next = false;
    int depth = 0;

    for (int i = 0; str[i]; ++i) {
        if (escape_next) {
            escape_next = false;
            continue;
        }
        if (str[i] == '\\') {
            escape_next = true;
            continue;
        }
        if (str[i] == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
        }
        else if (str[i] == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
        }
        if (in_single_quotes || in_double_quotes) continue;
        if (str[i] == '(') ++depth;
        else if (str[i] == ')') {
            if (--depth < 0) return true;
        }
    }
    return depth != 0;
}

TokenArray *tokenize(const char *input) {
    if (has_unclosed_quotes(input)) {
        fprintf(stderr, "Error: unclosed quote\n");
        exit(EXIT_FAILURE);
    }

    if (has_imbalanced_brackets(input)) {
        fprintf(stderr, "Error: imbalanced brackets\n");
        exit(EXIT_FAILURE);
    }

    Lexer *lexer = lexer_init(input);
    TokenArray *array = malloc(sizeof(TokenArray));
    array->tokens = NULL;
    array->length = 0;
    array->position = 0;

    Token *token;
    do {
        token = lexer_next_token(lexer);
        array->tokens = realloc(array->tokens, (array->length + 1) * sizeof(Token));
        array->tokens[array->length] = *token;
        if (token->value) {
            array->tokens[array->length].value = strdup(token->value); 
        }
        free_token(token);
    } while (array->tokens[array->length++].type != TOKEN_EOF);

    free_lexer(lexer);
    return array;
}

void free_token_array(TokenArray *array) {
    for (int i = 0; i < array->length; ++i) {
        if (array->tokens[i].value) free(array->tokens[i].value);
    }
    free(array->tokens);
    free(array);
}

Token *peek_token(TokenArray *array) {
    if (array->position >= array->length) return NULL;
    return &array->tokens[array->position];
}

Token *next_token(TokenArray *array) {
    if (array->position >= array->length) return NULL;
    return &array->tokens[array->position++];
}

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
        // if (token->type != TOKEN_RPAREN) {
        //     fprintf(stderr, "Error: expected ')'\n");
        //     exit(EXIT_FAILURE);
        // }
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
            // command_node->args = args;
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
    Token *token = peek_token(array);
    if (!token || token->type == TOKEN_EOF) {
        return NULL;
    }
    return parse_sequence_background(array);
}

void free_ast(ASTNode *node) {
    if (!node) return;

    if (node->left && node->left != node) free_ast(node->left);
    if (node->right && node->right != node) free_ast(node->right);

    if (node->args) {
        for (int i = 0; node->args[i]; ++i) free(node->args[i]);
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

    if (node->left && node->left != node) print_ast(node->left, level + 1);
    if (node->right && node->right != node) print_ast(node->right, level + 1);
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
    // test_parser("");
    // test_parser("echo 'Hello, world'");
    // test_parser("ls -l");
    // test_parser("ls -l | grep \"test\" > output.txt && (cd dir; ls) &");
    // test_parser("(ps aux; ls -a) && pwd");

    // incorrect stirngs
    // test_parser("ls -l | grep \"test\" > output.txt && (cd dir; ls &");
    // test_parser("ls -l | grep \"test\" > output.txt && (cd dir; ls) &");
    // test_parser("'");
    
    return 0;
}