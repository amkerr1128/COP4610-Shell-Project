#include "exec.h"
#include "path.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int parse_redirections(tokenlist *tokens, char ***out_argv, char **infile, char **outfile) {
    *infile = NULL; *outfile = NULL;
    size_t argc_est = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        const char *t = tokens->items[i];
        if (!strcmp(t, "<") || !strcmp(t, ">")) { i++; continue; }
        argc_est++;
    }
    if (!argc_est) return -1;

    char **argv = (char **)malloc(sizeof(char*) * (argc_est + 1));
    if (!argv) return -1;

    size_t k = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        char *t = tokens->items[i];
        if (!strcmp(t, "<")) {
            if (i + 1 >= tokens->size || *infile) { free(argv); return -1; }
            *infile = tokens->items[++i];
        } else if (!strcmp(t, ">")) {
            if (i + 1 >= tokens->size || *outfile) { free(argv); return -1; }
            *outfile = tokens->items[++i];
        } else {
            argv[k++] = t;
        }
    }
    argv[k] = NULL;
    *out_argv = argv;
    return (int)k;
}

static int open_infile_ro(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "Input: cannot open '%s': %s\n", path, strerror(errno)); return -1; }
    struct stat st;
    if (fstat(fd, &st) != 0) { fprintf(stderr, "Input: cannot stat '%s': %s\n", path, strerror(errno)); close(fd); return -1; }
    if (!S_ISREG(st.st_mode)) { fprintf(stderr, "Input: '%s' is not a regular file\n", path); close(fd); return -1; }
    return fd;
}

static int open_outfile_trunc_0600(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { fprintf(stderr, "Output: cannot open '%s': %s\n", path, strerror(errno)); return -1; }
    (void)fchmod(fd, 0600);
    return fd;
}

int execute_external_with_redir(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return -1;

    char **argv = NULL; char *infile = NULL; char *outfile = NULL;
    if (parse_redirections(tokens, &argv, &infile, &outfile) < 1) {
        fprintf(stderr, "Parse error in redirection.\n"); return -1;
    }

    const char *cmd = argv[0];
    char *prog_path = NULL;
    if (strchr(cmd, '/')) prog_path = strdup(cmd);
    else {
        prog_path = resolve_path(cmd);
        if (!prog_path) { fprintf(stderr, "%s: command not found\n", cmd); free(argv); return -1; }
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); free(argv); free(prog_path); return -1; }

    if (pid == 0) {
        if (infile) {
            int infd = open_infile_ro(infile);
            if (infd < 0) _exit(1);
            if (dup2(infd, STDIN_FILENO) < 0) { perror("dup2 stdin"); _exit(1); }
            close(infd);
        }
        if (outfile) {
            int outfd = open_outfile_trunc_0600(outfile);
            if (outfd < 0) _exit(1);
            if (dup2(outfd, STDOUT_FILENO) < 0) { perror("dup2 stdout"); _exit(1); }
            close(outfd);
        }
        execv(prog_path, argv);
        perror("execv");
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) perror("waitpid");
    free(argv); free(prog_path);
    return status;
}
