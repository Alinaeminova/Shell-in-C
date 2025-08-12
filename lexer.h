typedef enum {
    TOKEN_COMMAND,
    TOKEN_PIPE,
    TOKEN_INPUT_REDIR,
    TOKEN_OUTPUT_REDIR,
    TOKEN_APPEND_REDIR,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SEMICOLON,
    TOKEN_BACKGROUND,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    const char *input;
    int position;
    int length;
} Lexer;

Lexer *lexer_init(const char *input);
void free_lexer(Lexer *lexer);
void skip_whitespace(Lexer *lexer);
char *lexer_read_word(Lexer *lexer);
char *lexer_read_quotation(Lexer *lexer, char quote);
Token *lexer_next_token(Lexer *lexer);
void free_token(Token *token);
void test_lexer(const char *input);