#include "history.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define HISTORY_SIZE 10
static char *hist[HISTORY_SIZE];
static int count = 0;

void history_add(const char *line) {
    if (!line || !*line) return;
    if (count < HISTORY_SIZE) hist[count++] = strdup(line);
    else {
        free(hist[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) hist[i-1] = hist[i];
        hist[HISTORY_SIZE-1] = strdup(line);
    }
}

void history_print_last_three_and_free(void) {
    if (count == 0) {
        printf("No valid commands in history.\n");
    } else {
        int start = (count > 3) ? count - 3 : 0;
        for (int i = start; i < count; i++) printf("%s\n", hist[i]);
    }
    for (int i = 0; i < count; i++) free(hist[i]);
    count = 0;
}
