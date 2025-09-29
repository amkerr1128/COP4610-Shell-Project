#pragma once
#include <stdbool.h>
#include <sys/types.h>
#include "tokens.h"

typedef struct {
    int   job_num;
    pid_t pid;
    char *cmd_line;
    bool  is_done;
} background_job;

/* init + run + check + wait-all + print */
void bg_init(void);
int  execute_background(tokenlist *tokens); /* returns job number or -1 */
void bg_check_finished(void);
void bg_wait_all(void);
void bg_print_active(void);
