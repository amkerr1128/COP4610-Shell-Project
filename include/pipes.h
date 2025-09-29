#pragma once
#include "tokens.h"

/* Part 7: up to two pipes, falls back to single command path */
int execute_pipe_chain(tokenlist *tokens); /* returns exit status or -1 */
