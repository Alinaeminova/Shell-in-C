#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

Lexer *lexer_init(const char *input) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    return lexer;
}

void free_lexer(Lexer *lexer) {
    free(lexer);
}

void skip_whitespace(Lexer *lexer) {
    while (lexer->position < lexer->length && 
            isspace(lexer->input[lexer->position])) {
        lexer->position++;
    }
}

char *lexer_read_word(Lexer *lexer) {
    int start = lexer->position;
    while (lexer->position < lexer->length && 
            !isspace(lexer->input[lexer->position]) && 
            !strchr("|&;<>()", lexer->input[lexer->position])) {
        lexer->position++;
    }

    int length = lexer->position - start;
    char *value = malloc(length + 1);
    strncpy(value, lexer->input + start, length);
    value[length] = '\0';
    return value;
}

char *lexer_read_quotation(Lexer *lexer, char quote) {
    lexer->position++;
    int start = lexer->position;
    while (lexer->position < lexer->length && lexer->input[lexer->position] != quote) {
        lexer->position++;
    }

    // if (lexer->position >= lexer->length) {
    //     fprintf(stderr, "Error: unclosed quotation mark\n");
    //     exit(EXIT_FAILURE);
    // }

    int length = lexer->position - start;
    lexer->position++;

    char *value = malloc(length + 1);
    strncpy(value, lexer->input + start, length);
    value[length] = '\0';
    return value;
}

Token *lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);

    if (lexer->position >= lexer->length) {
        Token *token = malloc(sizeof(Token));
        token->type = TOKEN_EOF;
        token->value = NULL;
        return token;
    }

    char current = lexer->input[lexer->position];

    if (current == '"' || current == '\'') {
        char *value = lexer_read_quotation(lexer, current);

        Token *token = malloc(sizeof(Token));
        token->type = TOKEN_COMMAND;
        token->value = value;
        return token;
    }

    if (lexer->position + 1 < lexer->length) {
        char next = lexer->input[lexer->position + 1];

        if (current == '&' && next == '&') {
            Token *token = malloc(sizeof(Token));
            token->type = TOKEN_AND;
            token->value = NULL;
            lexer->position += 2;
            return token;
        }
        if (current == '|' && next == '|') {
            Token *token = malloc(sizeof(Token));
            token->type = TOKEN_OR;
            token->value = NULL;
            lexer->position += 2;
            return token;
        }
        if (current == '>' && next == '>') {
            Token *token = malloc(sizeof(Token));
            token->type = TOKEN_APPEND_REDIR;
            token->value = NULL;
            lexer->position += 2;
            return token;
        }
    }

    switch (current) {
        case '|':
            Token *pipe_token = malloc(sizeof(Token));
            pipe_token->type = TOKEN_PIPE;
            pipe_token->value = NULL;
            lexer->position++;
            return pipe_token;
        case '&':
            Token *bg_token = malloc(sizeof(Token));
            bg_token->type = TOKEN_BACKGROUND;
            bg_token->value = NULL;
            lexer->position++;
            return bg_token;
        case ';':
            Token *semi_token = malloc(sizeof(Token));
            semi_token->type = TOKEN_SEMICOLON;
            semi_token->value = NULL;
            lexer->position++;
            return semi_token;
        case '<':
            Token *input_token = malloc(sizeof(Token));
            input_token->type = TOKEN_INPUT_REDIR;
            input_token->value = NULL;
            lexer->position++;
            return input_token;
        case '>':
            Token *output_token = malloc(sizeof(Token));
            output_token->type = TOKEN_OUTPUT_REDIR;
            output_token->value = NULL;
            lexer->position++;
            return output_token;
        case '(':
            Token *lparen_token = malloc(sizeof(Token));
            lparen_token->type = TOKEN_LPAREN;
            lparen_token->value = NULL;
            lexer->position++;
            return lparen_token;
        case ')':
            Token *rparen_token = malloc(sizeof(Token));
            rparen_token->type = TOKEN_RPAREN;
            rparen_token->value = NULL;
            lexer->position++;
            return rparen_token;
    }

    if (isgraph(current)) {
        char *value = lexer_read_word(lexer);

        Token *token = malloc(sizeof(Token));
        token->type = TOKEN_COMMAND;
        token->value = value;
        return token;
    }

    fprintf(stderr, "Error: Unknown symbol %c\n", current);
    exit(EXIT_FAILURE);
}

void free_token(Token *token) {
    if (token->value) free(token->value);
    free(token);
}

// void test_lexer(const char *input) {
//     Lexer *lexer = lexer_init(input);
//     Token *token;
//     int token_type;

//     printf("Lexer string testing: \"%s\"\n", input);
//     do {
//         token = lexer_next_token(lexer);
//         token_type = token->type;
//         printf("Token: ");
//         switch (token_type) {
//             case TOKEN_COMMAND: printf("COMMAND '%s'", token->value); break;
//             case TOKEN_AND: printf("AND"); break;
//             case TOKEN_OR: printf("OR"); break;
//             case TOKEN_PIPE: printf("PIPE"); break;
//             case TOKEN_BACKGROUND: printf("BACKGROUND"); break;
//             case TOKEN_SEMICOLON: printf("SEMICOLON"); break;
//             case TOKEN_INPUT_REDIR: printf("INPUT_REDIR"); break;
//             case TOKEN_OUTPUT_REDIR: printf("OUTPUT_REDIR"); break;
//             case TOKEN_APPEND_REDIR: printf("APPEND_REDIR"); break;
//             case TOKEN_LPAREN: printf("LPAREN"); break;
//             case TOKEN_RPAREN: printf("RPAREN"); break;
//             case TOKEN_EOF: printf("EOF"); break;
//             default: printf("UNKNOWN"); break;
//         }
//         printf("\n");
//         free_token(token);
//     } while (token_type != TOKEN_EOF);
//     printf("\n");
//     free_lexer(lexer);
// }

// int main() {
//     test_lexer("echo 'Hello, world'");
//     test_lexer("ls -l");
//     test_lexer("ls -l | grep \"test\" > output.txt && (cd dir; ls) &");
//     test_lexer("cat < input.txt | sort | uniq >> 'output file.txt' || echo \"Error\"");
//     test_lexer("(ps aux; ls -a) && pwd");
//     return 0;
// }