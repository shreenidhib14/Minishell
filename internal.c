#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "variables.h"
#include "jobs.h"
#include "signal_handler.h"
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

void execute_internal(char *args[])
{
    char cwd[100];

    if (strcmp(args[0], "cd") == 0)
    {
        if (args[1] == NULL)
        {
            fprintf(stderr, "cd: missing argument\n");
            last_status = 1;
        }
        else if (chdir(args[1]) == -1)
        {
            perror("cd");
            last_status = 1;
        }
        else
        {
            last_status = 0;
        }
    }
    else if (strcmp(args[0], "echo") == 0)
    {
        for (int i = 1; args[i] != NULL; i++)
        {
            printf("%s ", args[i]);
        }

        printf("\n");
        last_status = 0;
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
            last_status = 0;
        }
        else
        {
            perror("pwd");
            last_status = 1;
        }
    }
    else if (strcmp(args[0], "jobs") == 0)
    { 
        reap_jobs();
        list_jobs();
        last_status = 0;
    }
    else if (strcmp(args[0], "bg") == 0)
{
    resume_background();
    last_status = 0;
}

    else if (strcmp(args[0], "bg") == 0)
    {
        resume_background();
        last_status = 0;
    }
    else if (strcmp(args[0], "fg") == 0)
    {
        pid_t pid = 0;

        if (args[1] != NULL)
        {
            char *endptr;
            long value;

            errno = 0;
            value = strtol(args[1], &endptr, 10);

            if (errno != 0 || *args[1] == '\0' ||
                *endptr != '\0' || value <= 0 || value > INT_MAX)
            {
                printf("fg: invalid PID: %s\n", args[1]);
                last_status = 1;
                return;
            }

            pid = (pid_t)value;
        }
        last_status = resume_foreground(pid);
    }

    else if (strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }

    else if (strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
}