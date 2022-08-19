#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
 *
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

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens;
	int i, background = 0;

	while (1)
	{
		bzero(line, sizeof(line));
		printf(">$ ");
		scanf("%[^\n]", line);
		getchar();

		while (waitpid(-1, NULL, WNOHANG) > 0)
		{
			printf("Shell: Background process finished\n");
		}

		line[strlen(line)] = '\n'; // terminate with new line
		tokens = tokenize(line);

		for (i = 0; tokens[i] != NULL; i++)
		{
			if (!strcmp(tokens[i], "&"))
			{
				background = 1;
				tokens[i] = NULL;
			}
		}

		if (tokens[0] == NULL)
		{
			free(tokens);
			continue;
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
			continue;
		}

		int fc = fork();
		if (fc < 0)
		{
			fprintf(stderr, "%s\n", "Unable to fork");
		}
		else if (fc == 0)
		{
			execvp(tokens[0], tokens);
			printf("Command execution failed\n");
			_exit(1);
		}
		else if (!background)
		{
			waitpid(fc, NULL, 0);
		}
		else
		{
			printf("Background process created with pid: %d\n", fc);
			background = 0;
		}

		// Freeing the allocated memory
		for (i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}
