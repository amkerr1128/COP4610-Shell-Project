#include "builtins.h"
#include "bg.h"
#include "history.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

void builtin_exit(void) {
    bg_wait_all();
    history_print_last_three_and_free();
    exit(0);
}

void builtin_cd(tokenlist *tokens) {
    char *target = NULL;

    if (tokens->size == 1) {
        target = getenv("HOME");
        if (!target) { fprintf(stderr, "cd: HOME not set\n"); return; }
    } else if (tokens->size == 2) {
        target = tokens->items[1];
    } else {
        fprintf(stderr, "cd: too many arguments\n"); return;
    }

    if (chdir(target) != 0) perror("cd");
    else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) setenv("PWD", cwd, 1);
    }
}

void builtin_jobs(void) {
    bg_print_active();
}
