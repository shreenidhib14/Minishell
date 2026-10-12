
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include "jobs.h"

Job job_list[MAX_JOBS];
int job_count = 0;

static int next_job_id = 1;

void add_job(pid_t pid, const char *command)
{
    if (job_count >= MAX_JOBS)
    {
        printf("jobs: job list is full\n");
        return;
    }

    job_list[job_count].job_id = next_job_id++;
    job_list[job_count].pid = pid;
    job_list[job_count].running = 1;

    snprintf(job_list[job_count].command,
             sizeof(job_list[job_count].command),
             "%s", command);

    job_count++;
}

void list_jobs(void)
{
    for (int i = 0; i < job_count; i++)
    {
        printf("[%d] %s %s [%d]\n",
               job_list[i].job_id,
               job_list[i].running ? "Running" : "Stopped",
               job_list[i].command,
               (int)job_list[i].pid);
    }
}

void remove_job(pid_t pid)
{
    for (int i = 0; i < job_count; i++)
    {
        if (job_list[i].pid == pid)
        {
            for (int j = i; j < job_count - 1; j++)
            {
                job_list[j] = job_list[j + 1];
            }

            job_count--;
            return;
        }
    }
}

void reap_jobs(void)
{
    int status;
    pid_t result;

    for (int i = 0; i < job_count; )
    {
        pid_t pid = job_list[i].pid;

        result = waitpid(pid, &status, WNOHANG | WUNTRACED);

        if (result == pid)
        {
            if (WIFEXITED(status))
            {
                printf("\n[%d] Done %s [%d] (exit status: %d)\n",
                       job_list[i].job_id,
                       job_list[i].command,
                       (int)pid,
                       WEXITSTATUS(status));

                remove_job(pid);
                continue;
            }
            else if (WIFSIGNALED(status))
            {
                printf("\n[%d] Done %s [%d] (exit status: %d)\n",
                       job_list[i].job_id,
                       job_list[i].command,
                       (int)pid,
                       128 + WTERMSIG(status));

                remove_job(pid);
                continue;
            }
            else if (WIFSTOPPED(status))
            {
                job_list[i].running = 0;
            }
        }

        i++;
    }
}

int resume_foreground(pid_t requested_pid)
{
    int status;
    pid_t pid;
    pid_t result;
    pid_t shell_pgid = getpgrp();
    int job_index = -1;

    if (job_count == 0)
    {
        printf("fg: no current job\n");
        return 1;
    }

    /* Plain fg selects the most recent job */
    if (requested_pid == 0)
    {
        job_index = job_count - 1;
    }
    else
    {
        for (int i = 0; i < job_count; i++)
        {
            if (job_list[i].pid == requested_pid)
            {
                job_index = i;
                break;
            }
        }
    }

    if (job_index == -1)
    {
        printf("fg: no such job: %d\n", (int)requested_pid);
        return 1;
    }

    pid = job_list[job_index].pid;

    printf("fg: bringing %s [%d] to foreground\n",
           job_list[job_index].command, (int)pid);

    /* Give the terminal to this job's process group */
    if (tcsetpgrp(STDIN_FILENO, pid) == -1)
    {
        perror("fg: tcsetpgrp");
        return 1;
    }

    /* Resume a stopped job */
    if (job_list[job_index].running == 0)
    {
        if (kill(-pid, SIGCONT) == -1)
        {
            perror("fg: SIGCONT");

            if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1)
            {
                perror("fg: restoring terminal");
            }

            return 1;
        }

        job_list[job_index].running = 1;
    }

    /* Wait until the job exits or stops */
    result = waitpid(pid, &status, WUNTRACED);

    /* Return terminal control to Minishell */
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1)
    {
        perror("fg: restoring terminal");
    }

    if (result == -1)
    {
        perror("fg: waitpid");
        return 1;
    }

    if (WIFEXITED(status))
    {
        int exit_status = WEXITSTATUS(status);

        remove_job(pid);

        printf("fg: process %d finished (exit status: %d)\n",
               (int)pid, exit_status);

        return exit_status;
    }
    else if (WIFSIGNALED(status))
    {
        int exit_status = 128 + WTERMSIG(status);

        remove_job(pid);

        printf("fg: process %d finished (exit status: %d)\n",
               (int)pid, exit_status);

        return exit_status;
    }
    else if (WIFSTOPPED(status))
    {
        for (int i = 0; i < job_count; i++)
        {
            if (job_list[i].pid == pid)
            {
                job_list[i].running = 0;
                break;
            }
        }

        printf("\nProcess stopped. PID: %d\n", (int)pid);
        return 128 + WSTOPSIG(status);
    }

    return 1;
}

void resume_background(void)
{
    for (int i = job_count - 1; i >= 0; i--)
    {
        if (job_list[i].running == 0)
        {
            if (kill(-job_list[i].pid, SIGCONT) == 0)
            {
                job_list[i].running = 1;

                printf("[%d] Running %s [%d]\n",
                       job_list[i].job_id,
                       job_list[i].command,
                       (int)job_list[i].pid);
            }
            else
            {
                perror("bg");
            }

            return;
        }
    }

    printf("bg: no stopped job\n");
}
