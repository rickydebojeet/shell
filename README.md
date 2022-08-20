# A Simple Shell Program
This is a simple shell program that can be used to understand how UNIX processes works.

## Features
### 1. Simple Functionality
* Ability to execute all simple linux commands like `ls, cat, echo, sleep, ps, etc.`
* Inbuilt `cd` command implementation to change directory.
* Inbuilt reaping mechanism to reap zombie processes.
### 2. Background Execution
* Ability to execute a command in background.
* Inbuilt reaping mechanism to reap zombie processes while taking a new command.
* Ability to run multiple commands in background.
### 3. Exit Command
* Ability to exit the shell using the `exit` command.
* Kills all foreground and background processes when exiting the shell.
* Cleans up all resources used by the shell (if any).
### 4. Signal Handling
* Ability to handle Ctrl+C signal.
* Terminates all the foreground processes when Ctrl+C is received.
* No effect on background processes.
### 5. Serial and Parallel Foreground Execution
* Ability to execute multiple foreground processes in parallel.
* Ability to execute multiple foreground processes in serial.

## Usage
To compile the program:
```bash
$ gcc my_shell.c -o my_shell
$ ./my_shell
```
* For background execution use `&` at the end of the command.
* For serial execution use `&&` at the end of the command.
* For parallel execution use `&&&` at the end of the command.

## Note
This is a simple shell program and does not support any complex commands.
