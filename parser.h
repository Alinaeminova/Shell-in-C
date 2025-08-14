#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct {
    Token *tokens;
    int length;
    int position;
} TokenArray;

TokenArray *tokenize(const char *input);
void free_token_array(TokenArray *array);
Token *peek_token(TokenArray *array);
Token *next_token(TokenArray *array);

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

#endif