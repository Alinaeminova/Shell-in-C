#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include "executor.h"

void execute_cd(ASTNode *node) {
    if (chdir(node->args[1])) perror("cd failed");
}

void execute_exit(int status) {
    exit(status);
}

int execute_command(ASTNode *node) {
    if (!node || node->type != NODE_COMMAND) return -1;

    if (node->input_file) {
        int fd = open(node->input_file, O_RDONLY);
        if (fd == -1) {
            perror("input file opening failed");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2 input failed");
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (node->output_file) {
        int flags = O_WRONLY | O_CREAT | (node->append ? O_APPEND : O_TRUNC);
        int fd = open(node->output_file, flags, 0644);
        if (fd == -1) {
            perror("output file opening failed");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 output failed");
            close(fd);
            return -1;
        }
        close(fd);
    }

    execvp(node->args[0], node->args);
    perror("execvp failed");
    return -1;
}

int execute_ast(ASTNode *node) {
    if (!node) return 0;

    switch (node->type) {
        case NODE_COMMAND: {
            if (node->args && !strcmp("cd", node->args[0])) {
                execute_cd(node);
            }

            else {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork failed");
                    return -1;
                }
                if (!pid) {
                    exit(execute_command(node));
                }
                int status;
                waitpid(pid, &status, 0);
                return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            }
        }

        case NODE_PIPE: {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                return -1;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("fork failed");
                close(pipefd[0]);
                close(pipefd[1]);
                return -1;
            }
            if (!pid1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                exit(execute_ast(node->left));
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("fork failed");
                close(pipefd[0]);
                close(pipefd[1]);
                return -1;
            }
            if (!pid2) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                exit(execute_ast(node->right));
            }

            close(pipefd[0]);
            close(pipefd[1]);

            int status1, status2;
            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);
            
            return WIFEXITED(status2) ? WEXITSTATUS(status2) : -1;
        }
        
        case NODE_AND: {
            int status1 = execute_ast(node->left);
            if (!status1) {
                return execute_ast(node->right);
            }
            return status1;
        }
        
        case NODE_OR: {
            int status1 = execute_ast(node->left);
            if (status1) {
                return execute_ast(node->right);
            }
            return status1;
        }
        
        case NODE_SEQUENCE: {
            execute_ast(node->left);
            return execute_ast(node->right);
        }
        
        case NODE_BACKGROUND: {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                return -1;
            }
            if (!pid) {
                setpgid(0, 0);
                exit(execute_ast(node->left));
            }

            printf("[%d]\n", pid);
            return 0;
        }
            
        case NODE_SUBSHELL: {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                return -1;
            }
            if (!pid) {
                setpgid(0, 0);
                exit(execute_ast(node->left));
            }
            
            int status;
            waitpid(pid, &status, 0);
            return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        }

        default:
            fprintf(stderr, "Unknown node type\n");
            return -1;
    }
}

void shell_loop() {
    char input[MAX_COMMAND_LENGTH];

    printf("Simple Shell (type 'exit' to quit)\n");

    while(1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        TokenArray *tokens = tokenize(input);
        ASTNode *ast = parse_expression(tokens);

        if (ast) {
            if (ast->args && !strcmp("exit", ast->args[0])) {
                free_ast(ast);
                free_token_array(tokens);
                execute_exit(0);
            }
            execute_ast(ast);
            free_ast(ast);
        }

        free_token_array(tokens);
    }
}

int main() {
    shell_loop();
    return 0;
}