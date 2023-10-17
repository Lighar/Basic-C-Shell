# Minishell Project Readme (Gauthier Rancoule)

## Introduction
This Git repository documents the steps I took to complete the Minishell Project for my 1st-year "Sciences du Numérique" course in 2023. The project involves creating a simplified command-line interpreter (shell) that offers basic Unix shell functionalities.

## Project Objectives
The main objectives of the Minishell Project are as follows:
- Develop a simplified command-line interpreter that can execute basic Unix shell commands.
- Progressively build the interpreter's functionality in a step-by-step manner, starting with a basic loop that reads and executes simple commands.
- Implement features such as process management, signal handling, I/O redirection, and pipelines.

## Project Steps

### 1. Main Loop
I created the basic behavior of the interpreter, which involves an infinite loop. Each iteration of this loop reads a line from the standard input, interprets it as a command, and launches a child process to execute that command.

### 2. Launching a Basic Command
I implemented the basic loop of the interpreter, limited to simple commands (without composition operators) and without patterns for file names.

### 3. Synchronization between the Shell and Its Children
I modified the code to make the interpreter wait for the completion of the last launched command before proceeding to read the next line.

### 4. Sequential Execution of Commands
I added two internal commands, "cd" and "exit," which are executed directly by the interpreter without launching child processes.

### 5. Launching Commands in the Background
I extended the initial code to allow launching commands in the background using the "&" symbol at the end of a command line.

### 6. Managing Processes Launched by the Shell
I added internal commands for process management, including listing jobs, stopping jobs, resuming background jobs, and bringing background jobs to the foreground.

### 7. Handling SIGTSTP and SIGINT
I modified the code to handle the SIGTSTP signal and suspend the active foreground process when the user presses Ctrl-Z.

### 8. Handling SIGINT
I implemented handling of the SIGINT signal to terminate the active foreground process when the user presses Ctrl-C.

## Architecture of the Application
The application is divided into five main components: Handlers, Command Managers in the list, Executors, Custom Commands, and the Main loop.

### Handlers
Three handlers are present in the project:
- The SIGCHLD handler that manages the state changes of child processes.
- The SIGINT handler that handles program interruption by the user using CTRL+C.
- The SIGSTOP handler that manages program termination by the user using CTRL+Z.

Only the SIGCHLD handler removes processes from the process list, while the other two only send signals to kill or suspend the process.

### Command Managers
Command managers are functions that handle commands in the list. They can add or remove commands. Their implementations are straightforward, except for the string copying when adding a command, which was a bit more complex.

### Executors
Two procedures allow command execution:
- The "command_execute" procedure, which executes a command, taking custom commands into account.
- The "fork_and_exec" procedure, which executes a command in a fork if it is not a custom command. Commands can be executed in the background or foreground. Pipes are managed by the "fork_and_exec" procedure.

### Custom Commands
To manage custom commands, procedures (lj, sj, bg, fg) needed to be created. Transitioning between foreground and background is handled by the "fg" and "bg" procedures, which change the "foreground_active" variable to determine if a process is in the foreground or not, along with the "is_backgrounded" variable in the command structure to know if a process is in the background or not.

### Main Loop
The main loop is an infinite loop that reads user commands and executes them.

## Design Choices and Specifics
### Handlers
Only the SIGCHLD handler removes processes from the process list to avoid conflicts with other handlers.

## Testing Methodology
I tested my program with basic commands, pipes, redirections, custom commands, signals, and commands in the background and foreground. Setting up automated tests proved challenging due to the structure of Minishell, so I conducted manual testing.

## Significant Tests
```
#Command without arguments (works)
$ ls
#Command with arguments (works)
$ ls -l
#Background command (works: the command is executed in the background and appears in the list of processes)
$ sleep 100 &
#Command with arguments and redirection (works: copies the input.txt file into an output.txt file or creates a new one if it doesn't exist)
$ cat < input.txt > output.txt
#Command with arguments and pipe (works: create two files toto.c and lulu.c, add some lines with 'int' in them, and run the command. Result: I have 5 'int' in both files)
$ cat toto.c lulu.c

## Usage guide

### Installation

Clone the repository and run the make command in the root directory of the repository.