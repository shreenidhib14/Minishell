#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

int check_pipe(char *args[])
{
    int i = 0;

    while(args[i] != NULL)
    {
        if(strcmp(args[i], "|") == 0)
        {
            return 1;
        }

        i++;
    }

    return 0;
}
int count_pipes(char *args[])
{
    int i = 0;
    int count = 0;

    while(args[i] != NULL)
    {
        if(strcmp(args[i], "|") == 0)
        {
            count++;
        }

        i++;
    }

    return count;
}
void execute_pipe(char *args[])
{
    int pipes = count_pipes(args);
    int commands = pipes + 1;

    int pipefd[2];
    int prev_fd = -1;
    int start = 0;

    pid_t pid;

    for(int i = 0; i < commands; i++)
    {
        if(i < commands - 1)
        {
            pipe(pipefd);
        }

        pid = fork();

        if(pid == 0)
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            /* Take input from previous pipe */
            if(prev_fd != -1)
            {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            /* Send output to next pipe */
            if(i < commands - 1)
            {
                dup2(pipefd[1], STDOUT_FILENO);

                close(pipefd[0]);
                close(pipefd[1]);
            }

            /* Find end of current command */
            int j = start;

            while(args[j] != NULL && strcmp(args[j], "|") != 0)
            {
                j++;
            }

            args[j] = NULL;

            execvp(args[start], &args[start]);

            perror("execvp");
            exit(1);
        }

        /* Parent */

        if(prev_fd != -1)
        {
            close(prev_fd);
        }

        if(i < commands - 1)
        {
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }

        /* Find next command */
        while(args[start] != NULL && strcmp(args[start], "|") != 0)
        {
            start++;
        }

        if(args[start] != NULL)
        {
            start++;
        }
    }

    if(prev_fd != -1)
    {
        close(prev_fd);
    }

    for(int i = 0; i < commands; i++)
    {
        wait(NULL);
    }
}
