#pragma once
#include <stdlib.h>

typedef struct {
    char **items;
    size_t size;
} tokenlist;

/* input + tokenization (unchanged behavior from your helpers) */
char *get_input(void);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
tokenlist *get_tokens(char *input);
void free_tokens(tokenlist *tokens);
