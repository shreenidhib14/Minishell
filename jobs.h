#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 100

typedef struct
{
    int job_id;
    pid_t pid;
    int running;
    char command[100];
} Job;

extern Job job_list[MAX_JOBS];
extern int job_count;

void add_job(pid_t pid, const char *command);
void list_jobs(void);
void remove_job(pid_t pid);
void reap_jobs(void);
void resume_background(void);
int resume_foreground(pid_t pid);
#endif
