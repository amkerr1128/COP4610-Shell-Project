#pragma once
#include "tokens.h"

/* Part 2 + 3 */
void expand_env_variables(tokenlist *tokens);
void expand_tilde(tokenlist *tokens);
