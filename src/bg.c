#include "bg.h"
#include "pipes.h"
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static background_job jobs[10];
static int next_job = 1;

void bg_init(void) {
    for (int i = 0; i < 10; i++) {
        jobs[i].job_num = 0; jobs[i].pid = 0; jobs[i].cmd_line = NULL; jobs[i].is_done = false;
    }
}

static int find_slot(void) {
    for (int i = 0; i < 10; i++) if (jobs[i].job_num == 0) return i;
    return -1;
}

static char *make_cmdline(tokenlist *toks) {
    if (!toks || toks->size == 0) return NULL;
    size_t len = 0; for (size_t i = 0; i < toks->size; i++) len += strlen(toks->items[i]) + 1;
    char *s = malloc(len); if (!s) return NULL; s[0] = '\0';
    for (size_t i = 0; i < toks->size; i++) { strcat(s, toks->items[i]); if (i+1<toks->size) strcat(s," "); }
    return s;
}

int execute_background(tokenlist *tokens) {
    int slot = find_slot();
    if (slot < 0) { fprintf(stderr, "Too many background jobs (max 10)\n"); return -1; }

    char *cmd = make_cmdline(tokens);
    if (!cmd) return -1;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); free(cmd); return -1; }

    if (pid == 0) { execute_pipe_chain(tokens); _exit(0); }

    jobs[slot].job_num = next_job++;
    jobs[slot].pid = pid;
    jobs[slot].cmd_line = cmd;
    jobs[slot].is_done = false;

    printf("[%d] %d\n", jobs[slot].job_num, pid);
    return jobs[slot].job_num;
}

void bg_check_finished(void) {
    for (int i = 0; i < 10; i++) {
        if (jobs[i].job_num > 0 && !jobs[i].is_done) {
            int status; pid_t r = waitpid(jobs[i].pid, &status, WNOHANG);
            if (r > 0) {
                printf("[%d]+ done %s\n", jobs[i].job_num, jobs[i].cmd_line);
                free(jobs[i].cmd_line);
                jobs[i].job_num = 0; jobs[i].pid = 0; jobs[i].cmd_line = NULL; jobs[i].is_done = true;
            }
        }
    }
}

void bg_wait_all(void) {
    for (int i = 0; i < 10; i++) if (jobs[i].pid > 0) waitpid(jobs[i].pid, NULL, 0);
}

void bg_print_active(void) {
    int any = 0;
    for (int i = 0; i < 10; i++) if (jobs[i].job_num > 0) {
        int status; pid_t r = waitpid(jobs[i].pid, &status, WNOHANG);
        if (r == 0) { printf("[%d]+ %d %s\n", jobs[i].job_num, jobs[i].pid, jobs[i].cmd_line); any = 1; }
    }
    if (!any) printf("No active background jobs.\n");
}
