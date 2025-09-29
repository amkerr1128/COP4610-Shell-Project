#include "tokens.h"
#include "expand.h"
#include "path.h"
#include "pipes.h"
#include "bg.h"
#include "builtins.h"
#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    bg_init();

    while (1) {
        char *user = getenv("USER");
        char *pwd  = getenv("PWD");
        char host[256];
        if (gethostname(host, sizeof(host)) != 0) {
            strncpy(host, "machine", sizeof(host)-1);
            host[sizeof(host)-1] = '\0';
        }

        if (user && pwd) printf("%s@%s:%s> ", user, host, pwd);
        else             printf("shell> ");
        fflush(stdout);

        char *input = get_input();
        if (!input) break;

        tokenlist *tokens = get_tokens(input);
        expand_env_variables(tokens);
        expand_tilde(tokens);

        if (tokens->size > 0) {
            const char *cmd = tokens->items[0];

            /* history records non-empty lines */
            if (strcmp(cmd, "") != 0) history_add(input);

            /* check trailing & */
            int background = 0;
            if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0) {
                background = 1;
                free(tokens->items[tokens->size - 1]);
                tokens->size--;
                tokens->items[tokens->size] = NULL;
            }

            if (is_builtin(cmd)) {
                if (strcmp(cmd, "exit") == 0)      builtin_exit();
                else if (strcmp(cmd, "cd") == 0)   builtin_cd(tokens);
                else if (strcmp(cmd, "jobs") == 0) builtin_jobs();
            } else {
                if (background) execute_background(tokens);
                else            execute_pipe_chain(tokens);
            }
        }

        bg_check_finished();
        free(input);
        free_tokens(tokens);
    }

    return 0;
}
