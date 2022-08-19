#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

#define BG_P 1
#define FG_P 0

char **tokenize(char *, char);
void prompt();
int command_handler(char **);
void execute_command(char **, int);
void int_handler_child(int);
void int_handler_parent(int);

int prompt_flag;
struct sigaction act_child;
struct sigaction act_parent;
pid_t pid;

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens;
	prompt_flag = 1;

	act_child.sa_handler = int_handler_child;
	act_parent.sa_handler = int_handler_parent;

	sigaction(SIGCHLD, &act_child, 0);
	sigaction(SIGINT, &act_parent, 0);

	while (1)
	{
		if (prompt_flag)
		{
			prompt();
		}

		bzero(line, sizeof(line));
		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n'; // terminate with new line
		tokens = tokenize(line, '\n');

		if (tokens[0] == NULL)
		{
			free(tokens);
			continue;
		}

		command_handler(tokens);
	}
	return 0;
}

/* Splits the string by delim and returns the array of tokens
 *
 */
char **tokenize(char *line, char delim)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	if (delim == '\n' || delim == ' ' || delim == '\t')
	{
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
	}
	else
	{
		for (i = 0; i < strlen(line); i++)
		{
			char readChar = line[i];
			if (readChar == delim)
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
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

void prompt()
{
	printf(">$ ");
}

int command_handler(char *tokens[])
{
	int i, process_type = FG_P;

	for (i = 0; tokens[i] != NULL; i++)
	{
		if (!strcmp(tokens[i], "&"))
		{
			process_type = BG_P;
			tokens[i] = NULL;
		}
	}

	if (!strcmp(tokens[0], "exit"))
	{
		kill(0, SIGKILL);
		exit(0);
	}
	else if (!strcmp(tokens[0], "cd"))
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

	execute_command(tokens, process_type);
	return 0;
}

void execute_command(char *tokens[], int process_type)
{
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "%s\n", "Unable to fork");
	}
	else if (pid == 0)
	{
		signal(SIGINT, SIG_IGN);
		execvp(tokens[0], tokens);
		printf("Command execution failed\n");
		_exit(1);
	}
	else if (!process_type)
	{
		waitpid(pid, NULL, 0);
	}
	else
	{
		printf("Background process created with pid: %d\n", pid);
		process_type = FG_P;
	}

	// Freeing the allocated memory
	for (int i = 0; tokens[i] != NULL; i++)
	{
		free(tokens[i]);
	}
	free(tokens);
}

void int_handler_child(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		printf("Shell: Background process finished\n");
	}
}

void int_handler_parent(int p)
{
	printf("Shell: Ctrl+C pressed\n");
}