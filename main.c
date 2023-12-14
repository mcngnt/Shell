#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "global.h"

#define MAX_LINE_LENGTH 1000

void trimLastCharacter(char *str)
{
    if (str == NULL || *str == '\0')
    {
        return;
    }
    size_t len = strlen(str);
    if (len > 0)
    {
        str[len - 1] = '\0';
    }
}

char** readLines()
{
    char** lines = NULL;
    char buffer[MAX_LINE_LENGTH];
    int lineCount = 0;
    lines = (char**)malloc(sizeof(char*));
    while (fgets(buffer, MAX_LINE_LENGTH, stdin) != NULL)
    {
        lines[lineCount] = (char*)malloc(strlen(buffer));
        trimLastCharacter(buffer);
        strcpy(lines[lineCount], buffer);
        lineCount++;
        lines = (char**)realloc(lines, (lineCount + 1) * sizeof(char*));
    }
    lines[lineCount] = NULL;
    return lines;
}

void freeLines(char** lines)
{
    for (int i = 0; lines[i] != NULL; i++)
    {
        free(lines[i]);
    }
    free(lines);
}


char* concat(char *s1, char *s2)
{
    char *result = (char*)malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void handle()
{
	rl_crlf();

	rl_replace_line("",0);
	rl_reset_line_state();
	rl_redisplay();
}



// declaration
int execute (struct cmd *cmd);

// name of the program, to be printed in several places
#define NAME "jean-mishell"


void exec_cd(struct cmd *cmd)
{
	char* targetdir = cmd->args[1];
	if(targetdir == NULL)
	{
		targetdir = getenv("HOME");
	}
	chdir(targetdir);
}

// Some helpful functions

void errmsg (char *msg)
{
	fprintf(stderr,"error: %s\n",msg);
}

// apply_redirects() should modify the file descriptors for standard
// input/output/error (0/1/2) of the current process to the files
// whose names are given in cmd->input/output/error.
// append is like output but the file should be extended rather
// than overwritten.

void apply_redirects(struct cmd *cmd)
{
	if(cmd->output != NULL)
	{
		int f = open(cmd->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    	dup2(f, STDOUT_FILENO);
    	close(f);
	}
	if(cmd->append != NULL)
	{
		int f = open(cmd->append, O_APPEND | O_WRONLY | O_CREAT, 0644);
    	dup2(f, STDOUT_FILENO);
    	close(f);
	}
	if(cmd->error != NULL)
	{
		int f = open(cmd->error, O_APPEND | O_WRONLY | O_CREAT, 0644);
    	dup2(f, 2);
    	close(f);
	}
	if(cmd->input != NULL)
	{
		int f = open(cmd->input, O_RDONLY, 0644);
    	dup2(f, STDIN_FILENO);
    	close(f);
	}
}

char* get_file_extension(char* filename)
{
    char* dot = strrchr(filename, '.');

    if (dot != NULL)
    {
        return dot + 1;
    }
    else 
    {
        return NULL;
    }
}

int find_wildcard(char** args, int* wcnb)
{
	int i = 0;
	while(args[i] != NULL)
	{
		if(strstr(args[i], "*.") != NULL)
		{
			*wcnb = i;
			return *wcnb;
		}
		++i;
	}
	return -1;
}

// The function execute() takes a command parsed at the command line.
// The structure of the command is explained in output.c.
// Returns the exit code of the command in question.


int execute (struct cmd *cmd)
{
	switch (cmd->type)
	{
	    case C_PLAIN:
	    	int wcnb;
	    	if(strcmp( cmd->args[0], "cd" ) == 0)
	    	{
	    		exec_cd(cmd);
	    		return 0;
	    	}
	    	else if (find_wildcard(cmd->args, &wcnb) != -1)
	    	{
	    		char* ext = strrchr(cmd->args[wcnb], '.');
	    		int fdestlen = (ext - cmd->args[wcnb]) - 1;
	    		char fdest[fdestlen + 1];
	    		strncpy(fdest, cmd->args[wcnb], fdestlen);
	    		fdest[fdestlen] = '\0';

	    		int p[2];
	    		pipe(p);

	    		if (fork() == 0)
	    		{
	    		  	dup2(p[1], STDOUT_FILENO);
    				size_t len = strlen("ls ") + strlen(cmd->args[wcnb]) + strlen(" | grep '.*\\") + strlen("$'");
    				char result[len];
    				strcpy(result, "ls ");
    				strncat(result, fdest, len - strlen(result) - 1);
    				strncat(result, " | grep '.*\\", len - strlen(result) - 1);
    				strncat(result, ext, len - strlen(result) - 1);
    				strncat(result, "$'", len - strlen(result) - 1);
	    		  	exit(execute(parser(result)));
	    		}
	    		else
	    		{
	    		 	close(p[1]);
	    		  	wait(NULL);
	    		  	if (fork() == 0)
	    		  	{
	    			  	dup2(p[0], STDIN_FILENO);
	    			  	char** args = readLines();
	    			  	int i = 0;
	    			  	while(args[i] != NULL)
	    			  	{
	    			  		i++;
	    			  	}
	    			  	char** newargs = malloc(sizeof(char*) * (i+2));
	    			  	char* narg = NULL;
	    			  	newargs[0] = cmd->args[0];
	    			  	newargs[i+1] = NULL;
	    			  	// strlen(args[i+1]);
	    			  	for (int j = 1; j < i+1; ++j)
	    			  	{
	    			  		narg = concat(fdest, args[j-1]);
	    			  		newargs[j] = narg;
	    			  	}

	    			  	cmd->args = newargs;
	    			  	int res = execute(cmd);
	    			  	freeLines(args);
	    			  	free(newargs);
	    			  	exit(res);
	    		  	}
	    			else
	    			{
	    		  		close(p[0]);
	    		  		int status;
	    		  		wait(&status);
	    		  		return WEXITSTATUS(status);
	    			}
	    		}
	    	}
	    	else
	    	{
	    		if (fork() == 0)
	    		{
	    			apply_redirects(cmd);
	    			execvp(cmd->args[0], cmd->args);
	    			printf("%s : command not found\n", cmd->args[0]);
	    			exit(-1);
	    		}
	    		else
	    		{
	    			int status;
	    			wait(&status);
	    			return WEXITSTATUS(status);
	    		}
	    	}
	    case C_SEQ:
	    	execute(cmd->left);
	    	return execute(cmd->right);
	    case C_AND:
	    	int rleftAND = execute(cmd->left);
	    	if(rleftAND == 0)
	    	{
	    		return execute(cmd->right);
	    	}
	    	else
	    	{
	    		return rleftAND;
	    	}
	    case C_OR:
	    	int rleftOR = execute(cmd->left);
	    	if(rleftOR != 0)
	    	{
	    		return execute(cmd->right);
	    	}
	    	else
	    	{
	    		return rleftOR;
	    	}
	    case C_PIPE:

			int p[2];
			pipe(p);

			if (fork() == 0)
			{
			  	dup2(p[1], STDOUT_FILENO);
			 	exit(execute(cmd->left));
			}
			else
			{
			 	close(p[1]);
			  	wait(NULL);
			  	if (fork() == 0)
			  	{
				  	dup2(p[0], STDIN_FILENO);
				  	exit(execute(cmd->right));
			  	}
				else
				{
			  		close(p[0]);
			  		int status;
			  		wait(&status);
			  		return WEXITSTATUS(status);
				}
			}
			return 0;

	    case C_VOID:
	    	if (fork() == 0)
	    	{
	    		apply_redirects(cmd);
	    		exit(execute(cmd->left));
	    	}
	    	else
	    	{
	    		int status;
	    		wait(&status);
	    		return WEXITSTATUS(status);
	    	}
		default:
			return -1;
	}

	return -1;
}

int main (int argc, char **argv)
{
	signal(SIGINT, handle);

	char *prompt = malloc(strlen(NAME)+3);
	printf("welcome to %s!\n", NAME);
	sprintf(prompt,"%s> ", NAME);

	while (1)
	{
		char *line = readline(prompt);
		if (!line) break;	// user pressed Ctrl+D; quit shell
		if (!*line) continue;	// empty line

		add_history (line);	// add line to history

		struct cmd *cmd = parser(line);
		if (!cmd) continue;	// some parse error occurred; ignore
		execute(cmd);
		// output(cmd,0);	// activate this for debugging
		// printf("%d return code\n",execute(cmd));
	}

	printf("goodbye!\n");
	return 0;
}
