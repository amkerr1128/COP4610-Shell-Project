#include "path.h"
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool is_builtin(const char *cmd) {
    return (strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "cd")   == 0 ||
            strcmp(cmd, "jobs") == 0);
}

static bool is_regular_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

static char *join_path(const char *dir, const char *cmd) {
    const char *base = (dir && *dir) ? dir : ".";
    char *out = (char *)malloc(strlen(base) + 1 + strlen(cmd) + 1);
    if (!out) return NULL;
    sprintf(out, "%s/%s", base, cmd);
    return out;
}

char *resolve_path(const char *cmd) {
    if (!cmd || !*cmd) return NULL;

    if (strchr(cmd, '/')) {
        char *cp = (char *)malloc(strlen(cmd) + 1);
        if (cp) strcpy(cp, cmd);
        return cp;
    }

    if (is_builtin(cmd)) return NULL;

    const char *path = getenv("PATH");
    if (!path || !*path) path = "/bin:/usr/bin";

    char *copy = (char *)malloc(strlen(path) + 1);
    if (!copy) return NULL;
    strcpy(copy, path);

    char *save = NULL;
    for (char *dir = strtok_r(copy, ":", &save); dir; dir = strtok_r(NULL, ":", &save)) {
        char *cand = join_path(dir, cmd);
        if (cand) {
            if (access(cand, X_OK) == 0 && is_regular_executable(cand)) {
                free(copy);
                return cand;
            }
            free(cand);
        }
    }
    free(copy);
    return NULL;
}
