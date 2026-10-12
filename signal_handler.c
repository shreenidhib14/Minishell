#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        write(STDOUT_FILENO, "\n", 1);
    }
    else if (sig == SIGQUIT)
    {
        write(STDOUT_FILENO, "\n", 1);
    }
}
