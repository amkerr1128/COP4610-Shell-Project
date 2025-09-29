#pragma once

/* simple fixed-size command history for Part 9 exit behavior */
void history_add(const char *cmd_line);
void history_print_last_three_and_free(void);
