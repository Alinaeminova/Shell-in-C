#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

#define MAX_COMMAND_LENGTH 1024

void execute_cd(ASTNode *node);
void execute_exit(int status);
int execute_command(ASTNode *node);
int execute_ast(ASTNode *node);
void shell_loop();

#endif