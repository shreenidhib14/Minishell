
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include "variables.h"
#include "jobs.h"

void execute_external(char *args[])
{
    pid_t pid;
    pid_t shell_pgid = getpgrp();
    int background = 0;
    int status;
    int i = 0;

    /* Count arguments */
    while (args[i] != NULL)
    {
        i++;
    }

    /* Check for background operator (&) */
    if (i > 0 && strcmp(args[i - 1], "&") == 0)
    {
        background = 1;
        args[i - 1] = NULL;
    }

    pid = fork();

    if (pid == 0)
    {
        /* Child process */
        setpgid(0, 0);

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        execvp(args[0], args);

        perror("execvp");
        exit(127);
    }
    else if (pid > 0)
    {
        /* Parent process */
        setpgid(pid, pid);

        if (background)
        {
            int previous_job_count = job_count;

            add_job(pid, args[0]);

            if (job_count > previous_job_count)
            {
                printf("[%d] %d\n",
                       job_list[job_count - 1].job_id,
                       (int)pid);

                last_status = 0;
            }
            else
            {
                kill(pid, SIGTERM);
                waitpid(pid, NULL, 0);
                last_status = 1;
            }

            return;
        }

        /* Give terminal control to foreground child */
        if (tcsetpgrp(STDIN_FILENO, pid) == -1)
        {
            perror("tcsetpgrp");
        }

        /* Wait until foreground child exits or stops */
        if (waitpid(pid, &status, WUNTRACED) == -1)
        {
            perror("waitpid");
            last_status = 1;
        }
        else if (WIFEXITED(status))
        {
            last_status = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            last_status = 128 + WTERMSIG(status);
        }
        else if (WIFSTOPPED(status))
        {
            int previous_job_count = job_count;

            add_job(pid, args[0]);

            if (job_count > previous_job_count)
            {
                job_list[job_count - 1].running = 0;

                printf("\nProcess stopped. PID: %d\n",
                       (int)pid);

                last_status = 128 + WSTOPSIG(status);
            }
            else
            {
                kill(-pid, SIGKILL);
                waitpid(pid, NULL, 0);
                last_status = 1;
            }
        }

        /* Return terminal control to the shell */
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1)
        {
            perror("tcsetpgrp");
        }
    }
    else
    {
        perror("fork");
        last_status = 1;
    }
}
