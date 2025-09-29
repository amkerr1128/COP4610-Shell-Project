#include "pipes.h"
#include "exec.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

static int parse_pipe_commands(tokenlist *tokens, tokenlist **commands, int *num) {
    *num = 0;
    int pipes = 0;
    for (size_t i = 0; i < tokens->size; i++)
        if (strcmp(tokens->items[i], "|") == 0) pipes++;
    if (pipes > 2) { fprintf(stderr, "Too many pipes (max 2 allowed)\n"); return -1; }

    *num = pipes + 1;
    int idx = 0;
    commands[0] = new_tokenlist();

    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "|") == 0) {
            idx++;
            commands[idx] = new_tokenlist();
        } else {
            add_token(commands[idx], tokens->items[i]);
        }
    }
    return 0;
}

int execute_pipe_chain(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return -1;

    tokenlist *cmds[3];
    int ncmd = 0;
    if (parse_pipe_commands(tokens, cmds, &ncmd) < 0) return -1;

    if (ncmd == 1) {
        int r = execute_external_with_redir(cmds[0]);
        free_tokens(cmds[0]);
        return r;
    }

    int pipes[2][2];
    for (int i = 0; i < ncmd - 1; i++) if (pipe(pipes[i]) < 0) { perror("pipe"); return -1; }

    pid_t pids[3];
    for (int i = 0; i < ncmd; i++) {
        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); return -1; }

        if (pids[i] == 0) {
            if (i > 0)  dup2(pipes[i-1][0], STDIN_FILENO);
            if (i < ncmd - 1) dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < ncmd - 1; j++) { close(pipes[j][0]); close(pipes[j][1]); }

            execute_external_with_redir(cmds[i]);
            _exit(0);
        }
    }

    for (int i = 0; i < ncmd - 1; i++) { close(pipes[i][0]); close(pipes[i][1]); }

    int status = 0;
    for (int i = 0; i < ncmd; i++) waitpid(pids[i], &status, 0);

    for (int i = 0; i < ncmd; i++) free_tokens(cmds[i]);
    return WEXITSTATUS(status);
}
