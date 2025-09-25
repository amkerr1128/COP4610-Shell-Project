#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For gethostname()

void expand_env_variables(tokenlist *tokens)
{
    for (int i = 0; i < (int)tokens->size; i++) {
        char *tok = tokens->items[i];
        if (tok[0] == '$' && tok[1] != '\0') {
            const char *varname = tok + 1;
            const char *val = getenv(varname);
            if (val == NULL) val = ""; /* replace with empty string if unset */
            
            /* Replace the token with the environment variable value */
            size_t newlen = strlen(val);
            char *newbuf = (char *)malloc(newlen + 1);
            strcpy(newbuf, val);
            free(tokens->items[i]);
            tokens->items[i] = newbuf;
        }
    }
}

void expand_tilde(tokenlist *tokens) {
    // Get the home directory path from the environment variables
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        // If HOME is not set, we can't do expansion, so just return.
        return;
    }

    // Loop through each token in the list
    for (int i = 0; i < tokens->size; i++) {
        // Case 1: The token is exactly "~"
        if (strcmp(tokens->items[i], "~") == 0) {
            free(tokens->items[i]); // Free the old "~" string
            tokens->items[i] = (char *)malloc(strlen(home_dir) + 1);
            strcpy(tokens->items[i], home_dir); // Replace with the home directory path
        }
        // Case 2: The token starts with "~/"
        else if (strncmp(tokens->items[i], "~/", 2) == 0) {
            // Construct the new, expanded path
            char *rest_of_path = tokens->items[i] + 1; // Get the path part after the "~"
            int new_path_len = strlen(home_dir) + strlen(rest_of_path) + 1;
            char *new_path = (char *)malloc(new_path_len);
            
            // Copy the home directory and the rest of the path into the new string
            strcpy(new_path, home_dir);
            strcat(new_path, rest_of_path);

            free(tokens->items[i]); // Free the old "~/path" string
            tokens->items[i] = new_path; // Replace with the new, full path
        }
    }
}

int main()
{
	while (1) {
		char *user = getenv("USER");
        char *pwd = getenv("PWD");
        char hostname[256]; // Buffer to hold the machine's name

        // gethostname() is more reliable than getenv("HOSTNAME") or getenv("MACHINE")
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strncpy(hostname, "machine", sizeof(hostname) - 1); // Fallback name
            hostname[sizeof(hostname) - 1] = '\0';
        }

        // Check that environment variables exist before printing
        if (user && pwd) {
            printf("%s@%s:%s> ", user, hostname, pwd);
        } else {
            printf("shell> "); // Fallback prompt
        }
        fflush(stdout); // Ensures the prompt appears immediately

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		if (input == NULL) {
			/* EOF reached, exit the shell */
			break;
		}
		
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		
		/* Expand environment variables in tokens */
		expand_env_variables(tokens);

		/* Expand tildes in tokens */
        expand_tilde(tokens);
		
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}

char *get_input(void) {
	char *buffer = NULL;
	int bufsize = 0;
	char line[5];
	
	/* Handle EOF case - if fgets returns NULL immediately, return NULL */
	if (fgets(line, 5, stdin) == NULL) {
		return NULL;
	}
	
	/* Process the first line */
	int addby = 0;
	char *newln = strchr(line, '\n');
	if (newln != NULL)
		addby = newln - line;
	else
		addby = 5 - 1;
	buffer = (char *)realloc(buffer, bufsize + addby);
	memcpy(&buffer[bufsize], line, addby);
	bufsize += addby;
	
	/* Continue reading if no newline found (line was too long) */
	while (newln == NULL && fgets(line, 5, stdin) != NULL)
	{
		addby = 0;
		newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
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
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item) {
	int i = tokens->size;

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
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
