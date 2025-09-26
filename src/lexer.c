#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // gethostname(), access()
#include <sys/stat.h>   // stat()
#include <errno.h>
#include <limits.h>     // PATH_MAX
#include <stdbool.h>

/* -------------------- Part 2: env expansion -------------------- */
void expand_env_variables(tokenlist *tokens)
{
    for (int i = 0; i < (int)tokens->size; i++) {
        char *tok = tokens->items[i];
        if (tok[0] == '$' && tok[1] != '\0') {
            const char *varname = tok + 1;
            const char *val = getenv(varname);
            if (val == NULL) val = ""; /* replace with empty string if unset */

            size_t newlen = strlen(val);
            char *newbuf = (char *)malloc(newlen + 1);
            strcpy(newbuf, val);
            free(tokens->items[i]);
            tokens->items[i] = newbuf;
        }
    }
}

/* -------------------- Part 3: tilde expansion -------------------- */
void expand_tilde(tokenlist *tokens) {
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) return;

    for (int i = 0; i < (int)tokens->size; i++) {
        if (strcmp(tokens->items[i], "~") == 0) {
            free(tokens->items[i]);
            tokens->items[i] = (char *)malloc(strlen(home_dir) + 1);
            strcpy(tokens->items[i], home_dir);
        } else if (strncmp(tokens->items[i], "~/", 2) == 0) {
            char *rest_of_path = tokens->items[i] + 1; // keep the '/'
            size_t newlen = strlen(home_dir) + strlen(rest_of_path) + 1;
            char *new_path = (char *)malloc(newlen);
            strcpy(new_path, home_dir);
            strcat(new_path, rest_of_path);
            free(tokens->items[i]);
            tokens->items[i] = new_path;
        }
    }
}

/* -------------------- Part 4: PATH search -------------------- */

static bool is_regular_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;
    // access(X_OK) already checked by caller; this double-checks type
    return true;
}

static char *join_path(const char *dir, const char *cmd) {
    /* Handles empty dir as "." (current directory) */
    const char *base = (dir && *dir) ? dir : ".";
    size_t len = strlen(base) + 1 /* '/' */ + strlen(cmd) + 1 /* '\0' */;
    char *out = (char *)malloc(len);
    if (!out) return NULL;
    snprintf(out, len, "%s/%s", base, cmd);
    return out;
}

bool is_builtin(const char *cmd) {
    /* Placeholder for Part 9; update when built-ins are implemented */
    (void)cmd;
    return false;
}

char *resolve_path(const char *cmd) {
    if (!cmd || !*cmd) return NULL;

    /* If command contains '/', don't search PATH—use as-is */
    if (strchr(cmd, '/')) {
        /* Caller may want a private copy */
        char *cp = (char *)malloc(strlen(cmd) + 1);
        if (cp) strcpy(cp, cmd);
        return cp;
    }

    if (is_builtin(cmd)) {
        /* Built-ins are handled internally, not via PATH */
        return NULL;
    }

    const char *path = getenv("PATH");
    if (!path || !*path) path = "/bin:/usr/bin";

    char *path_copy = (char *)malloc(strlen(path) + 1);
    if (!path_copy) return NULL;
    strcpy(path_copy, path);

    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        char *cand = join_path(dir, cmd);
        if (cand) {
            if (access(cand, X_OK) == 0 && is_regular_executable(cand)) {
                free(path_copy);
                return cand; /* caller frees */
            }
            free(cand);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return NULL; /* not found */
}

/* -------------------- Main loop -------------------- */

int main()
{
    while (1) {
        char *user = getenv("USER");
        char *pwd = getenv("PWD");
        char hostname[256];

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strncpy(hostname, "machine", sizeof(hostname) - 1);
            hostname[sizeof(hostname) - 1] = '\0';
        }

        if (user && pwd) printf("%s@%s:%s> ", user, hostname, pwd);
        else             printf("shell> ");
        fflush(stdout);

        char *input = get_input();
        if (input == NULL) break; /* EOF */

        /* Debug: echo raw line */
        /* printf("whole input: %s\n", input); */

        tokenlist *tokens = get_tokens(input);

        /* Expansions (Parts 2–3) */
        expand_env_variables(tokens);
        expand_tilde(tokens);

        /* Debug: show tokens post-expansion */
        for (int i = 0; i < (int)tokens->size; i++) {
            printf("token %d: (%s)\n", i, tokens->items[i]);
        }

        /* ---- Part 4: PATH search demonstration ----
         * If there is at least one token (command) and it does not contain '/',
         * try resolving via PATH and report result (or error).
         * (Execution will be added in later parts.)
         */
        if (tokens->size > 0) {
            const char *cmd = tokens->items[0];

            if (!is_builtin(cmd)) {
                /* Only search PATH for non-builtins with no '/' */
                if (strchr(cmd, '/') == NULL) {
                    char *resolved = resolve_path(cmd);
                    if (resolved) {
                        printf("resolved command: %s\n", resolved);
                        free(resolved);
                    } else {
                        fprintf(stderr, "%s: command not found\n", cmd);
                    }
                } else {
                    /* Has '/' — treat as direct path (exists/executable checked later) */
                    printf("command has explicit path: %s\n", cmd);
                }
            }
        }

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

/* -------------------- Provided helpers -------------------- */

char *get_input(void) {
    char *buffer = NULL;
    int bufsize = 0;
    char line[5];

    if (fgets(line, 5, stdin) == NULL) {
        return NULL;
    }

    int addby = 0;
    char *newln = strchr(line, '\n');
    if (newln != NULL)
        addby = (int)(newln - line);
    else
        addby = 5 - 1;
    buffer = (char *)realloc(buffer, bufsize + addby);
    memcpy(&buffer[bufsize], line, addby);
    bufsize += addby;

    while (newln == NULL && fgets(line, 5, stdin) != NULL)
    {
        addby = 0;
        newln = strchr(line, '\n');
        if (newln != NULL)
            addby = (int)(newln - line);
        else
            addby = 5 - 1;
        buffer = (char *)realloc(buffer, bufsize + addby);
        memcpy(&buffer[bufsize], line, addby);
        bufsize += addby;
        if (newln != NULL)
            break;
    }

    buffer = (char *)realloc(buffer, bufsize + 1);
    buffer[bufsize] = 0;
    return buffer;
}

tokenlist *new_tokenlist(void) {
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **)malloc(sizeof(char *));
    tokens->items[0] = NULL;
    return tokens;
}

void add_token(tokenlist *tokens, char *item) {
    int i = (int)tokens->size;

    tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *)malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);

    tokens->size += 1;
}

tokenlist *get_tokens(char *input) {
    char *buf = (char *)malloc(strlen(input) + 1);
    strcpy(buf, input);
    tokenlist *tokens = new_tokenlist();
    char *tok = strtok(buf, " ");
    while (tok != NULL)
    {
        add_token(tokens, tok);
        tok = strtok(NULL, " ");
    }
    free(buf);
    return tokens;
}

void free_tokens(tokenlist *tokens) {
    for (int i = 0; i < (int)tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}
