#pragma once
#include "tokens.h"

/* Part 9 built-ins */
void builtin_exit(void);               /* waits bg jobs, prints last 3, exits */
void builtin_cd(tokenlist *tokens);    /* cd PATH | cd (HOME) */
void builtin_jobs(void);               /* prints active background jobs */
