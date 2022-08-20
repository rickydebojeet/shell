#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

enum command_type
{
	BACKGROUND_COMMAND,
	FOREGROUND_COMMAND,
	SERIAL_COMMAND,
	PARALLEL_COMMAND
};

char **tokenize(char *);
void execute_command(char **, enum command_type);
void int_handler_parent(int);
void program_closer(char **);
void program_closer_temp(char **, char **);
void change_directory(char **);

int prompt_flag;			 // 1 for prompt, 0 for no prompt
struct sigaction act_parent; // signal handler for parent
pid_t pid;					 // pid of child process
int proc[64];				 // array of pids of background processes

int main(int argc, char *argv[])
{
	pid_t shellpid = getpid();
	char line[MAX_INPUT_SIZE];
	char **tokens;
	prompt_flag = 1; // prompt by default
	int i, process_type;

	for (i = 0; i < 64; i++)
	{
		proc[i] = -1;
	}

	act_parent.sa_handler = int_handler_parent; // set signal handler for parent

	sigaction(SIGINT, &act_parent, 0); // set interrupt signal handler for parent

	setpgid(shellpid, shellpid); // set shell as process group leader

	while (1)
	{
		// Check for zombie background processes and reap it
		for (int i = 0; i < 64; i++)
		{
			if (proc[i] != -1 && waitpid(proc[i], NULL, WNOHANG) > 0)
			{
				printf("Shell: Background process finished with pid: %d\n", proc[i]);
				proc[i] = -1;
			}
		}
		process_type = FOREGROUND_COMMAND; // default process type
		if (prompt_flag)
		{
			printf(">$ ");
		}

		bzero(line, sizeof(line)); // clear line
		scanf("%[^\n]", line);	   // read line
		getchar();				   // read newline

		line[strlen(line)] = '\n'; // terminate with new line
		prompt_flag = 1;
		tokens = tokenize(line);

		// finding the command type
		for (i = 0; tokens[i] != NULL; i++)
		{
			if (!strcmp(tokens[i], "&"))
			{
				process_type = BACKGROUND_COMMAND;
				tokens[i] = NULL;
			}
			else if (!strcmp(tokens[i], "&&"))
			{
				process_type = SERIAL_COMMAND;
				tokens[i] = NULL;
			}
			else if (!strcmp(tokens[i], "&&&"))
			{
				process_type = PARALLEL_COMMAND;
				tokens[i] = NULL;
			}
		}

		// Blank line
		if (tokens[0] == NULL)
		{
			free(tokens);
			continue;
		}
		else if (!strcmp(tokens[0], "exit"))
		{
			program_closer(tokens);
		}
		// Parallel Foreground Execution
		else if (process_type == PARALLEL_COMMAND)
		{
			// Iterating through the commands
			char *command = strtok(line, "&&&");
			while (command != NULL)
			{
				// Tokenizing the command
				char **temp_tokens = tokenize(command);
				if (temp_tokens[0] == NULL || temp_tokens[0][0] == ' ')
				{
					free(temp_tokens);
					break;
				}
				// Changing the directory if cd is called
				if (!strcmp(temp_tokens[0], "cd"))
				{
					change_directory(temp_tokens);
				}
				else if (!strcmp(temp_tokens[0], "exit"))
				{
					program_closer_temp(temp_tokens, tokens);
				}
				else
				{
					pid = fork();
					// Error in forking
					if (pid < 0)
					{
						fprintf(stderr, "%s\n", "Unable to fork");
					}
					// Child Process
					else if (pid == 0)
					{
						// Ignore SIGINT so that parent can handle it
						signal(SIGINT, SIG_IGN);
						execvp(temp_tokens[0], temp_tokens);
						printf("Command execution failed\n");
						_exit(1);
					}
					// Parent Process
					// Doesn't waits for child process to finish; instead stores the pid in an array
					else
					{
						for (int j = 0; j < 64; j++)
						{
							if (proc[j] == -1)
							{
								proc[j] = pid;
								break;
							}
						}
					}
				}

				// Freeing the memory
				for (i = 0; temp_tokens[i] != NULL; i++)
				{
					free(temp_tokens[i]);
				}
				free(temp_tokens);

				command = strtok(NULL, "&&&");
			}

			// Waiting for the child processes to finish; so that parent can reap it
			for (i = 0; i < 64; i++)
			{
				if (proc[i] != -1)
				{
					waitpid(proc[i], NULL, 0);
					proc[i] = -1;
				}
			}
		}
		// Serial Foreground Execution
		else if (process_type == SERIAL_COMMAND)
		{
			// Iterating through the commands and executing them one by one
			char *command = strtok(line, "&&");
			while (command != NULL)
			{
				// Tokenizing the command
				char **temp_tokens = tokenize(command);
				if (temp_tokens[0] == NULL || temp_tokens[0][0] == ' ')
				{
					free(temp_tokens);
					break;
				}
				// Changing the directory if cd is called
				else if (!strcmp(temp_tokens[0], "cd"))
				{
					change_directory(temp_tokens);
				}
				else if (!strcmp(temp_tokens[0], "exit"))
				{
					program_closer_temp(temp_tokens, tokens);
				}
				// Executing the command normally
				else
				{
					execute_command(temp_tokens, process_type);
				}

				// Freeing the memory
				for (i = 0; temp_tokens[i] != NULL; i++)
				{
					free(temp_tokens[i]);
				}
				free(temp_tokens);

				command = strtok(NULL, "&&");
			}
		}
		else if (!strcmp(tokens[0], "cd"))
		{
			change_directory(tokens);
		}
		// Background or Foreground Command
		else
		{
			execute_command(tokens, process_type);
		}

		// Freeing the allocated memory
		for (int i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}

/* Function to tokenize the input line.
 * The functions splits the input line into tokens and returns the array of tokens.
 *
 * @param line Input from stdin
 * @return array of tokens
 */
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

/*
 * Function to change the current working directory.
 * Uses the chdir() system call to change the current working directory.
 * If the directory is not found, prints an error message.
 *
 * @param tokens Array of tokens
 */
void change_directory(char *tokens[])
{
	if (tokens[1] == NULL)
	{
		printf("Shell: Incorrect command\n");
	}
	else if (chdir(tokens[1]))
	{
		printf("Shell: Incorrect command\n");
	}
}

/*
 * Function to execute the command.
 * Uses the execvp() system call to execute the command.
 * If the command is not found, prints an error message.
 *
 * @param tokens Array of tokens
 * @param process_type Type of process
 */
void execute_command(char *tokens[], enum command_type process_type)
{
	pid = fork();
	// Error in forking
	if (pid < 0)
	{
		fprintf(stderr, "%s\n", "Unable to fork");
	}
	// Child Process
	else if (pid == 0)
	{
		// If the command is not background process, set the process group id to
		// the child process id. This will allow the parent to kill it on Ctrl+C.
		if (process_type != BACKGROUND_COMMAND)
		{
			setpgid(0, 0);
		}
		// Ignore the SIGINT signal. This will allow the parent to kill the child
		signal(SIGINT, SIG_IGN);
		execvp(tokens[0], tokens);
		printf("Command execution failed\n");
		_exit(1);
	}
	// Not a Bacground Process; wait for the child process to finish and reap it
	else if (process_type != BACKGROUND_COMMAND)
	{
		waitpid(pid, NULL, 0);
	}
	// Background Process; do not wait for the child process to finish
	// instead store the pid in an array to reap it later
	else
	{
		for (int j = 0; j < 64; j++)
		{
			if (proc[j] == -1)
			{
				proc[j] = pid;
				break;
			}
		}
		printf("Background process created with pid: %d\n", pid);
	}
}

/*
 * Function to close the program.
 * Kills all the child processes and exits the program.
 * Cleans up the allocated memory.
 * Uses the exit() system call to close the program.
 *
 * @param tokens Array of tokens
 */
void program_closer(char *tokens[])
{
	int i;
	// Kill all the background child processes
	for (i = 0; i < 64; i++)
	{
		if (proc[i] != -1)
		{
			kill(proc[i], SIGKILL);
			proc[i] = -1;
		}
	}
	// Free the allocated memory
	for (i = 0; tokens[i] != NULL; i++)
	{
		free(tokens[i]);
	}
	free(tokens);
	exit(0);
}

/*
 * Function to close the program while in parallel or serial execution.
 * Utilizies the program_closer() function to close the program.
 *
 * @param temp_tokens Array of temp tokens
 * @param tokens Array of tokens
 */
void program_closer_temp(char *temp_tokens[], char *tokens[])
{
	int i;
	for (i = 0; temp_tokens[i] != NULL; i++)
	{
		free(temp_tokens[i]);
	}
	free(temp_tokens);
	program_closer(tokens);
}

/*
 * Function to handle the SIGINT signal.
 * Kills all the foreground child processes and reaps them.
 * Returns the control to the shell.
 */
void int_handler_parent(int p)
{
	// Kill all the foreground child processes using the process group id
	if (kill(-pid, SIGTERM) == 0)
	{
		printf("Shell: Killing child process\n");
		// Reap the child processes
		if (waitpid(pid, NULL, 0) > 0)
		{
			printf("Shell: Reaping done. Press Enter\n");
		}
		prompt_flag = 0;
	}
}