#include "expand.h"
#include <string.h>
#include <stdlib.h>

void expand_env_variables(tokenlist *tokens) {
    for (int i = 0; i < (int)tokens->size; i++) {
        char *tok = tokens->items[i];
        if (tok[0] == '$' && tok[1] != '\0') {
            const char *varname = tok + 1;
            const char *val = getenv(varname);
            if (val == NULL) val = "";
            char *newbuf = (char *)malloc(strlen(val) + 1);
            if (!newbuf) continue;
            strcpy(newbuf, val);
            free(tokens->items[i]);
            tokens->items[i] = newbuf;
        }
    }
}

void expand_tilde(tokenlist *tokens) {
    char *home = getenv("HOME");
    if (!home) return;
    for (int i = 0; i < (int)tokens->size; i++) {
        if (strcmp(tokens->items[i], "~") == 0) {
            free(tokens->items[i]);
            tokens->items[i] = (char *)malloc(strlen(home) + 1);
            if (!tokens->items[i]) continue;
            strcpy(tokens->items[i], home);
        } else if (strncmp(tokens->items[i], "~/", 2) == 0) {
            char *rest = tokens->items[i] + 1;
            char *np = (char *)malloc(strlen(home) + strlen(rest) + 1);
            if (!np) continue;
            strcpy(np, home);
            strcat(np, rest);
            free(tokens->items[i]);
            tokens->items[i] = np;
        }
    }
}
