#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "signal_handler.h"
#include "variables.h"
#include "jobs.h"

void execute_internal(char *args[]);
void execute_external(char *args[]);
int check_pipe(char *args[]);
int count_pipes(char *args[]);
void execute_pipe(char *args[]);
void expand_variables(char *args[]);


int parse_input(char *input, char *args[])
{
    static char parsed[100];

    char *src = input;
    char *dst = parsed;
    int i = 0;

    while (*src != '\0')
    {
        while (*src == ' ' || *src == '\t')
            src++;

        if (*src == '\0')
            break;

        if (i >= 49)
            break;

        args[i++] = dst;
        char quote = '\0';

        while (*src != '\0')
        {
            if (quote == '\0' &&
                (*src == ' ' || *src == '\t'))
                break;

            if (quote == '\0' &&
                (*src == '\'' || *src == '"'))
            {
                quote = *src++;
                continue;
            }

            if (quote != '\0' && *src == quote)
            {
                quote = '\0';
                src++;
                continue;
            }

            *dst++ = *src++;
        }

        if (quote != '\0')
        {
            printf("msh: unmatched quote\n");
            args[0] = NULL;
            return 1;
        }

                *dst++ = '\0';

        while (*src == ' ' || *src == '\t')
            src++;
    }
    args[i] = NULL;
    return 0;
}

int main()
{
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    char input[100];
    char *args[50];
    char prompt[100] = "msh>";
    while(1)
    {
        reap_jobs();
        printf("%s ", prompt);
        if(fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        if (parse_input(input, args) != 0)
        continue;
        if(args[0] == NULL)
        {
            continue;
        }
        if (strncmp(args[0], "PS1=", 4) == 0)
        {
            strcpy(prompt, args[0] + 4);
            continue;
        }
        expand_variables(args);
        if(check_pipe(args))
        {
            execute_pipe(args);
            continue;
        }

        if(strcmp(args[0], "cd") == 0 ||
           strcmp(args[0], "echo") == 0 ||
           strcmp(args[0], "pwd") == 0 ||
           strcmp(args[0], "exit") == 0 ||
            strcmp(args[0], "jobs") == 0 ||
            strcmp(args[0], "bg") == 0 ||
            strcmp(args[0], "fg") == 0)
        {
            execute_internal(args);
            continue;
        }
        execute_external(args);
    }

    return 0;
}