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

/* Part 7: Piping */
int execute_pipe_chain(tokenlist *tokens); /* returns status or -1 */

/* Part 8: Background processing */
typedef struct {
    int job_num;
    pid_t pid;
    char *cmd_line;
    bool is_done;
} background_job;

int execute_background(tokenlist *tokens); /* returns job number or -1 */
void check_background_jobs(void); /* check for completed jobs */
