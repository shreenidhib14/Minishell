#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "variables.h"

int last_status = 0;

void expand_variables(char *args[])
{
    static char expanded[50][100];
    int i = 0;

    while (args[i] != NULL)
    {
        if (strcmp(args[i], "$?") == 0)
        {
            snprintf(expanded[i], sizeof(expanded[i]),
                     "%d", last_status);
            args[i] = expanded[i];
        }
        else if (strcmp(args[i], "$$") == 0)
        {
            snprintf(expanded[i], sizeof(expanded[i]),
                     "%d", getpid());
            args[i] = expanded[i];
        }
        else if (strcmp(args[i], "$SHELL") == 0)
{
    snprintf(expanded[i], sizeof(expanded[i]),
             "/mnt/c/minishell/minishell");
    args[i] = expanded[i];
}
else if (args[i][0] == '$')
{
    char *value = getenv(args[i] + 1);

    if (value != NULL)
    {
        snprintf(expanded[i], sizeof(expanded[i]),
                 "%s", value);
        args[i] = expanded[i];
    }
    else
    {
        args[i] = "";
    }
}

        i++;
    }
}
