#pragma once
#include <stdbool.h>

/* built-in detection used by main and path search */
bool is_builtin(const char *cmd);

/* Part 4: $PATH search (returns malloc'd string or NULL) */
char *resolve_path(const char *cmd);
