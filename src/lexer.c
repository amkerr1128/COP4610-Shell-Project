#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // gethostname(), access(), fork(), execv(), dup2(), close()
#include <sys/stat.h>   // stat(), fstat(), S_ISREG
#include <sys/wait.h>   // waitpid()
#include <fcntl.h>      // open(), O_*
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

/* -------------------- Part 9: Command History -------------------- */
#define HISTORY_SIZE 10
static char *command_history[HISTORY_SIZE];
static int history_count = 0;

static void add_to_history(const char *cmd_line) {
    if (history_count < HISTORY_SIZE) {
        command_history[history_count++] = strdup(cmd_line);
    } else {
        free(command_history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            command_history[i - 1] = command_history[i];
        }
        command_history[HISTORY_SIZE - 1] = strdup(cmd_line);
    }
}

/* -------------------- Part 2: env expansion -------------------- */
void expand_env_variables(tokenlist *tokens)
{
    for (int i = 0; i < (int)tokens->size; i++) {
        char *tok = tokens->items[i];
        if (tok[0] == '$' && tok[1] != '\0') {
            const char *varname = tok + 1;
            const char *val = getenv(varname);
            if (val == NULL) val = ""; /* empty if unset */

            size_t newlen = strlen(val);
            char *newbuf = (char *)malloc(newlen + 1);
            if (!newbuf) continue;
            strcpy(newbuf, val);
            free(tokens->items[i]);
            tokens->items[i] = newbuf;
        }
    }
}

/* -------------------- Part 3: tilde expansion -------------------- */
void expand_tilde(tokenlist *tokens) {
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) return;

    for (int i = 0; i < (int)tokens->size; i++) {
        if (strcmp(tokens->items[i], "~") == 0) {
            free(tokens->items[i]);
            tokens->items[i] = (char *)malloc(strlen(home_dir) + 1);
            if (!tokens->items[i]) continue;
            strcpy(tokens->items[i], home_dir);
        } else if (strncmp(tokens->items[i], "~/", 2) == 0) {
            char *rest_of_path = tokens->items[i] + 1; // keep the '/'
            size_t newlen = strlen(home_dir) + strlen(rest_of_path) + 1;
            char *new_path = (char *)malloc(newlen);
            if (!new_path) continue;
            strcpy(new_path, home_dir);
            strcat(new_path, rest_of_path);
            free(tokens->items[i]);
            tokens->items[i] = new_path;
        }
    }
}

/* -------------------- Part 4: PATH search -------------------- */

static bool is_regular_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;
    return true;
}

static char *join_path(const char *dir, const char *cmd) {
    const char *base = (dir && *dir) ? dir : ".";
    size_t len = strlen(base) + 1 /* '/' */ + strlen(cmd) + 1;
    char *out = (char *)malloc(len);
    if (!out) return NULL;
    snprintf(out, len, "%s/%s", base, cmd);
    return out;
}

bool is_builtin(const char *cmd) {
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "cd") == 0 || strcmp(cmd, "jobs") == 0) {
        return true;
    }
    return false;
}

char *resolve_path(const char *cmd) {
    if (!cmd || !*cmd) return NULL;

    if (strchr(cmd, '/')) {
        char *cp = (char *)malloc(strlen(cmd) + 1);
        if (cp) strcpy(cp, cmd);
        return cp;
    }

    if (is_builtin(cmd)) {
        return NULL; /* built-ins handled internally later */
    }

    const char *path = getenv("PATH");
    if (!path || !*path) path = "/bin:/usr/bin";

    char *path_copy = (char *)malloc(strlen(path) + 1);
    if (!path_copy) return NULL;
    strcpy(path_copy, path);

    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        char *cand = join_path(dir, cmd);
        if (cand) {
            if (access(cand, X_OK) == 0 && is_regular_executable(cand)) {
                free(path_copy);
                return cand; /* caller frees */
            }
            free(cand);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return NULL;
}

/* -------------------- Part 6: I/O redirection -------------------- */

/* Parse argv + find < and > filenames. Builds a new argv (without redir tokens). */
static int parse_redirections(tokenlist *tokens,
                              char ***out_argv,
                              char **infile,
                              char **outfile)
{
    *infile = NULL;
    *outfile = NULL;

    // First pass: count argv entries excluding redir tokens and their args
    size_t argc_est = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        const char *t = tokens->items[i];
        if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0) {
            i++; // skip filename
            continue;
        }
        argc_est++;
    }

    if (argc_est == 0) return -1;

    char **argv = (char **)malloc(sizeof(char*) * (argc_est + 1));
    if (!argv) return -1;

    size_t k = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        char *t = tokens->items[i];
        if (strcmp(t, "<") == 0) {
            if (i + 1 >= tokens->size) { free(argv); return -1; }
            if (*infile != NULL) { free(argv); return -1; } // multiple inputs not supported
            *infile = tokens->items[i+1];
            i++;
        } else if (strcmp(t, ">") == 0) {
            if (i + 1 >= tokens->size) { free(argv); return -1; }
            if (*outfile != NULL) { free(argv); return -1; } // multiple outputs not supported
            *outfile = tokens->items[i+1];
            i++;
        } else {
            argv[k++] = t; // just point into tokens storage
        }
    }
    argv[k] = NULL;
    *out_argv = argv;
    return (int)k;
}

static int open_infile_ro(const char *path) {
    // Must exist and be a regular file; open read-only, do not modify
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Input: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        fprintf(stderr, "Input: cannot stat '%s': %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Input: '%s' is not a regular file\n", path);
        close(fd);
        return -1;
    }
    return fd;
}

static int open_outfile_trunc_0600(const char *path) {
    // Create/overwrite regular file, perms 0600 (-rw-------)
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        fprintf(stderr, "Output: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }
    // Ensure mode is exactly 0600 (in case umask altered it)
    if (fchmod(fd, 0600) != 0) {
        // Not fatal, but warn
        // fprintf(stderr, "Warning: could not set mode 0600 on '%s'\n", path);
    }
    return fd;
}

int execute_external_with_redir(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return -1;

    char **argv = NULL;
    char *infile = NULL;
    char *outfile = NULL;

    if (parse_redirections(tokens, &argv, &infile, &outfile) < 1) {
        fprintf(stderr, "Parse error in redirection.\n");
        return -1;
    }

    const char *cmd = argv[0];
    char *prog_path = NULL;

    if (strchr(cmd, '/')) {
        prog_path = strdup(cmd);
    } else {
        prog_path = resolve_path(cmd);
        if (!prog_path) {
            fprintf(stderr, "%s: command not found\n", cmd);
            free(argv);
            return -1;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(argv);
        free(prog_path);
        return -1;
    }

    if (pid == 0) {
        /* --- Child: set up redirections in correct order (stdin first, then stdout) --- */
        int infd = -1, outfd = -1;

        if (infile) {
            infd = open_infile_ro(infile);
            if (infd < 0) _exit(1);
            if (dup2(infd, STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                _exit(1);
            }
            close(infd);
        }

        if (outfile) {
            outfd = open_outfile_trunc_0600(outfile);
            if (outfd < 0) _exit(1);
            if (dup2(outfd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                _exit(1);
            }
            close(outfd);
        }

        execv(prog_path, argv);
        perror("execv");
        _exit(127);
    }

    /* --- Parent: wait (foreground) --- */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
    }

    free(argv);
    free(prog_path);
    return status;
}

/* -------------------- Part 7: Piping -------------------- */

/* Parse tokens into separate commands separated by | */
static int parse_pipe_commands(tokenlist *tokens, tokenlist **commands, int *num_commands) {
    *num_commands = 0;
    
    // Count pipe separators
    int pipe_count = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "|") == 0) {
            pipe_count++;
        }
    }
    
    // Maximum 2 pipes allowed (3 commands total)
    if (pipe_count > 2) {
        fprintf(stderr, "Too many pipes (max 2 allowed)\n");
        return -1;
    }
    
    *num_commands = pipe_count + 1;
    commands[0] = new_tokenlist();
    
    int cmd_idx = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "|") == 0) {
            cmd_idx++;
            commands[cmd_idx] = new_tokenlist();
        } else {
            add_token(commands[cmd_idx], tokens->items[i]);
        }
    }
    
    return 0;
}

/* Execute a chain of piped commands */
int execute_pipe_chain(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return -1;
    
    tokenlist *commands[3]; // Max 3 commands (2 pipes)
    int num_commands = 0;
    
    if (parse_pipe_commands(tokens, commands, &num_commands) < 0) {
        return -1;
    }
    
    if (num_commands == 1) {
        // No pipes, just execute normally
        int result = execute_external_with_redir(commands[0]);
        free_tokens(commands[0]);
        return result;
    }
    
    // Set up pipes
    int pipes[2][2]; // [pipe_index][read_fd, write_fd]
    
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (int j = 0; j < num_commands; j++) {
                free_tokens(commands[j]);
            }
            return -1;
        }
    }
    
    // Fork processes for each command
    pid_t pids[3];
    
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            // Clean up pipes
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for (int j = 0; j < num_commands; j++) {
                free_tokens(commands[j]);
            }
            return -1;
        }
        
        if (pids[i] == 0) {
            // Child process
            // Set up input redirection
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    _exit(1);
                }
            }
            
            // Set up output redirection
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    _exit(1);
                }
            }
            
            // Close all pipe fds
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execute_external_with_redir(commands[i]);
            _exit(0);
        }
    }
    
    // Parent: close all pipe fds
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    int status;
    for (int i = 0; i < num_commands; i++) {
        waitpid(pids[i], &status, 0);
    }
    
    // Clean up command tokens
    for (int i = 0; i < num_commands; i++) {
        free_tokens(commands[i]);
    }
    
    return WEXITSTATUS(status);
}

/* -------------------- Part 8: Background Processing -------------------- */

static background_job bg_jobs[10]; // Max 10 background jobs
static int next_job_num = 1;

/* Initialize background job tracking */
static void init_background_jobs(void) {
    for (int i = 0; i < 10; i++) {
        bg_jobs[i].job_num = 0;
        bg_jobs[i].pid = 0;
        bg_jobs[i].cmd_line = NULL;
        bg_jobs[i].is_done = false;
    }
}

/* Find next available job slot */
static int find_job_slot(void) {
    for (int i = 0; i < 10; i++) {
        if (bg_jobs[i].job_num == 0) {
            return i;
        }
    }
    return -1; // No available slots
}

/* Create command line string from tokens */
static char *create_cmd_line(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return NULL;
    
    size_t total_len = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        total_len += strlen(tokens->items[i]) + 1; // +1 for space
    }
    
    char *cmd_line = malloc(total_len);
    if (!cmd_line) return NULL;
    
    cmd_line[0] = '\0';
    for (size_t i = 0; i < tokens->size; i++) {
        strcat(cmd_line, tokens->items[i]);
        if (i < tokens->size - 1) {
            strcat(cmd_line, " ");
        }
    }
    
    return cmd_line;
}

/* Execute command in background */
int execute_background(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return -1;
    
    int slot = find_job_slot();
    if (slot < 0) {
        fprintf(stderr, "Too many background jobs (max 10)\n");
        return -1;
    }
    
    // Create command line for display
    char *cmd_line = create_cmd_line(tokens);
    if (!cmd_line) return -1;
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(cmd_line);
        return -1;
    }
    
    if (pid == 0) {
        // Child process - execute the command
        execute_pipe_chain(tokens);
        _exit(0);
    }
    
    // Parent process - track the job
    bg_jobs[slot].job_num = next_job_num++;
    bg_jobs[slot].pid = pid;
    bg_jobs[slot].cmd_line = cmd_line;
    bg_jobs[slot].is_done = false;
    
    printf("[%d] %d\n", bg_jobs[slot].job_num, pid);
    
    return bg_jobs[slot].job_num;
}

/* Check for completed background jobs */
void check_background_jobs(void) {
    for (int i = 0; i < 10; i++) {
        if (bg_jobs[i].job_num > 0 && !bg_jobs[i].is_done) {
            int status;
            pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);
            
            if (result > 0) {
                // Job completed
                printf("[%d]+ done %s\n", bg_jobs[i].job_num, bg_jobs[i].cmd_line);
                free(bg_jobs[i].cmd_line);
                bg_jobs[i].job_num = 0;
                bg_jobs[i].pid = 0;
                bg_jobs[i].cmd_line = NULL;
                bg_jobs[i].is_done = true;
            }
        }
    }
}

/* -------------------- Part 9: Internal Commands -------------------- */

static void builtin_exit(void) {
    // Wait for all background jobs to finish
    for (int i = 0; i < 10; i++) {
        if (bg_jobs[i].pid > 0) {
            waitpid(bg_jobs[i].pid, NULL, 0);
        }
    }

    // Display command history
    printf("Last commands:\n");
    if (history_count == 0) {
        printf("No valid commands in history.\n");
    } else {
        int start = (history_count > 3) ? history_count - 3 : 0;
        for (int i = start; i < history_count; i++) {
            printf("%s\n", command_history[i]);
        }
    }
    
    // Free history memory before exiting
    for (int i = 0; i < history_count; i++) {
        free(command_history[i]);
    }

    exit(0);
}

static void builtin_cd(tokenlist *tokens) {
    char *target_dir = NULL;

    if (tokens->size == 1) {
        // No arguments, cd to HOME
        target_dir = getenv("HOME");
        if (!target_dir) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    } else if (tokens->size == 2) {
        // One argument
        target_dir = tokens->items[1];
    } else {
        // Too many arguments
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }

    if (chdir(target_dir) != 0) {
        // chdir failed, print error
        perror("cd");
    } else {
        // On success, update the PWD environment variable
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("PWD", cwd, 1);
        }
    }
}

static void builtin_jobs(void) {
    bool found_job = false;
    for (int i = 0; i < 10; i++) {
        if (bg_jobs[i].job_num > 0) {
            // Check if the job is still running before printing
            int status;
            pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);
            if (result == 0) {
                 printf("[%d]+ %d %s\n", bg_jobs[i].job_num, bg_jobs[i].pid, bg_jobs[i].cmd_line);
                 found_job = true;
            }
        }
    }
    if (!found_job) {
        printf("No active background jobs.\n");
    }
}

/* -------------------- Main loop -------------------- */

int main()
{
    /* Initialize background job tracking */
    init_background_jobs();

    while (1) {
        char *user = getenv("USER");
        char *pwd = getenv("PWD");
        char hostname[256];

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strncpy(hostname, "machine", sizeof(hostname) - 1);
            hostname[sizeof(hostname) - 1] = '\0';
        }

        if (user && pwd) printf("%s@%s:%s> ", user, hostname, pwd);
        else             printf("shell> ");
        fflush(stdout);

        char *input = get_input();
        if (input == NULL) break; /* EOF (Ctrl+D) */

        tokenlist *tokens = get_tokens(input);

        /* Parts 2–3 expansions */
        expand_env_variables(tokens);
        expand_tilde(tokens);

        if (tokens->size > 0) {
            const char *cmd = tokens->items[0];

            // Add valid commands (non-empty) to history
            if (strcmp(cmd, "") != 0) {
                add_to_history(input);
            }

            bool is_background = false;
            if (tokens->size > 0 && strcmp(tokens->items[tokens->size - 1], "&") == 0) {
                is_background = true;
                // Remove & from tokens
                free(tokens->items[tokens->size - 1]);
                tokens->size--;
                tokens->items[tokens->size] = NULL;
            }

            if (is_builtin(cmd)) {
                // --- NEW: Handle built-in commands ---
                if (strcmp(cmd, "exit") == 0) {
                    builtin_exit();
                } else if (strcmp(cmd, "cd") == 0) {
                    builtin_cd(tokens);
                } else if (strcmp(cmd, "jobs") == 0) {
                    builtin_jobs();
                }
            } else {
                if (is_background) {
                    execute_background(tokens);
                } else {
                    execute_pipe_chain(tokens);
                }
            }
        }

        /* Check for completed background jobs */
        check_background_jobs();

        free(input);
        free_tokens(tokens);
    }

    return 0;
}

/* -------------------- Provided helpers (unchanged) -------------------- */

char *get_input(void) {
	char *buffer = NULL;
	int bufsize = 0;
	char line[5];

    if (fgets(line, 5, stdin) == NULL) {
        return NULL;
    }

		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
        addby = (int)(newln - line);
    else
        addby = 5 - 1;
    buffer = (char *)realloc(buffer, bufsize + addby);
    memcpy(&buffer[bufsize], line, addby);
    bufsize += addby;

    while (newln == NULL && fgets(line, 5, stdin) != NULL)
    {
        addby = 0;
        newln = strchr(line, '\n');
        if (newln != NULL)
            addby = (int)(newln - line);
		else
			addby = 5 - 1;
		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;
		if (newln != NULL)
			break;
	}

	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;
	return buffer;
}

tokenlist *new_tokenlist(void) {
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
    tokens->items[0] = NULL;
	return tokens;
}

void add_token(tokenlist *tokens, char *item) {
    int i = (int)tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

tokenlist *get_tokens(char *input) {
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input);
	tokenlist *tokens = new_tokenlist();
	char *tok = strtok(buf, " ");
	while (tok != NULL)
	{
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}
	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens) {
    for (int i = 0; i < (int)tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
