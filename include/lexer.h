#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char ** items;
    size_t size;
} tokenlist;

char * get_input(void);
tokenlist * get_tokens(char *input);
tokenlist * new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

/* Part 2–3 */
void expand_env_variables(tokenlist *tokens);
void expand_tilde(tokenlist *tokens);

/* Part 4: PATH search */
char *resolve_path(const char *cmd);  /* returns malloc'd string or NULL */
bool  is_builtin(const char *cmd);    /* placeholder (false for now) */

/* Part 6: I/O redirection */
int execute_external_with_redir(tokenlist *tokens); /* returns child status or -1 */
