# Shell

This project is a custom shell implementation for a Linux environment, created for the COP4610 Operating Systems course. It supports essential shell features including I/O redirection, piping, background processing, and built-in commands like `cd`, `jobs`, and `exit`.

## Group Members
- **Austin Kerr**: amk21u@fsu.edu
- **Jackson Luther**: jbl22h@fsu.edu
- **Connor Cook**: cjc22c@fsu.edu
## Division of Labor

### Part 1: Prompt
- **Responsibilities**: Display a prompt showing the username, machine name, and current absolute working directory in the format USER@MACHINE:PWD> by expanding the corresponding environment variables. 
- **Assigned to**: Connor Cook

### Part 2: Environment Variables
- **Responsibilities**: Expand tokens that begin with a dollar sign ($) into their corresponding environment variable values before command execution.
- **Assigned to**: Jane Smith

### Part 3: Tilde Expansion
- **Responsibilities**: Expand tokens that are a standalone tilde (~) or begin with ~/ into the absolute path of the $HOME environment variable. 
- **Assigned to**: Alex Brown

### Part 4: $PATH Search
- **Responsibilities**: For any command that does not contain a slash (/), search the directories listed in the $PATH environment variable to locate the executable. If the executable is not found, display a "command not found" error. 
- **Assigned to**: Alex Brown, Jane Smith

### Part 5: External Command Execution
- **Responsibilities**: Execute external commands and their arguments by creating a child process with fork() and running the command within the child process using execv(). 
- **Assigned to**: Alex Brown, Jane Smith

### Part 6: I/O Redirection
- **Responsibilities**: Implement I/O redirection for standard input from a file (<) and standard output to a file (>). Output redirection must create or overwrite files with 

-rw------- permissions.
- **Assigned to**: Jane Smith

### Part 7: Piping
- **Responsibilities**: Implement command piping (|) for up to two pipes, redirecting the standard output of one command to the standard input of the next. Each command in the pipeline must run in its own process.
- **Assigned to**: John Doe

### Part 8: Background Processing
- **Responsibilities**: Implement background processing for commands ending with an ampersand (&). The shell should not wait for the command to finish and must print the job number and PID upon start, and a completion message when done.
- **Assigned to**: Alex Brown, John Doe

### Part 9: Internal Command Execution
- **Responsibilities**: Implement the built-in shell commands exit, cd, and jobs with all specified functionality, including argument handling and error reporting. 
- **Assigned to**: Alex Brown

### Part 10: External Timeout Executable
- **Responsibilities**: Create a separate utility program that takes a timeout duration and a command as arguments, executes the command, and terminates it if it runs longer than the specified duration.
- **Assigned to**: Alex Brown, Jane Smith

### Extra Credit
- **Responsibilities**: Implement support for an unlimited number of pipes, allow piping and I/O redirection to be used within the same command, and enable the shell to be executed recursively from within itself.
- **Assigned to**: Alex Brown

## File Listing
```
shell/
│
├── src/
│ ├── main.c
│ └── shell.c
│
├── include/
│ └── shell.h
│
├── README.md
└── Makefile
```
## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Austin Kerr

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

### Jackson Luther

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


### Connor Cook

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2025-09-23 | Initial project setup, Git repository configuration, and README updates.  |
| 2025-09-23 | Completed Part 0: Tokenization and Part 1: Prompt.  |
| YYYY-MM-DD | [Description of task]  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |



## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Extra Credit
- **Extra Credit 1**: [Extra Credit Option]
- **Extra Credit 2**: [Extra Credit Option]
- **Extra Credit 3**: [Extra Credit Option]

## Considerations
[Description]
