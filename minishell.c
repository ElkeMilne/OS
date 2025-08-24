/*********************************************************************
   Program  : miniShell                   Version    : 1.7
 --------------------------------------------------------------------
   Fully functional Linux/Unix command line interpreter
   Fixes cd command, backgroundFlag jobs, delayed job reporting,
   proper error handling, and child termination on exec failure.
 --------------------------------------------------------------------
   File       : minishell.c
   Compiler/System : gcc/linux
********************************************************************/
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20           /* max number of command tokens */
#define NL 100          /* input buffer size */
#define MAX_JOBS 100    /* maximum number of backgroundFlag jobs */

char line[NL];          /* command input buffer */

typedef struct {
    int job_id;
    pid_t pid;
    char command[NL];
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;
int job_id_counter = 1;  /* Auto-incremented job ID */
Job finished_jobs[MAX_JOBS];
int finished_job_count = 0;

/*
    signal handler for SIGCHLD
    This will report when a backgroundFlag process finishes
*/
void sighandler(int sig)
{
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                /* store finished job */
                finished_jobs[finished_job_count++] = jobs[i];

                // Remove the job from the list
                for (int j = i; j < job_count - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
                break;
            }
        }
    }
}

/* prints finished backgroundFlag jobs after prompt */
void printCompletedJobs()
{
    for (int i = 0; i < finished_job_count; i++) {
        fprintf(stdout, "[%d]+ Done %s\n", finished_jobs[i].job_id, finished_jobs[i].command);
        fflush(stdout);
    }
    finished_job_count = 0;  // Clear finished jobs after printing
}

int main(int argk, char *argv[], char *envp[])
{
    int frkRtnVal;      /* value returned by fork sys call */
    char *v[NV];        /* array of pointers to command line tokens */
    char *sep = " \t\n";/* command line token separators    */
    int i;              /* parse index */
    int backgroundFlag;     /* backgroundFlag process flag */

    signal(SIGCHLD, sighandler);
    
    while (1) {
        fgets(line, NL, stdin);
        fflush(stdin);

        // Print finished jobs after receiving any input
        printCompletedJobs();

        if (feof(stdin)) {
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;
        }

        // copy line to another variable to use later 
        char line_copy[NL];
        strcpy(line_copy, line);
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }
        v[i] = NULL;

        backgroundFlag = 0;
        if (i > 1 && strcmp(v[i - 1], "&") == 0) {
            backgroundFlag = 1;
            v[i - 1] = NULL;
            line_copy[strcspn(line_copy, "&")] = 0; // remove trailing '&'
        }
        
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                if (chdir(getenv("HOME")) != 0) perror("cd failed");
            } else {
                if (chdir(v[1]) != 0) perror("cd failed");
            }
            continue;
        }

        /* fork a child process to execute command */
        switch (frkRtnVal = fork()) {
            case -1: {
                perror("fork failed");
                break;
            }
            case 0: {
                execvp(v[0], v);
                perror("exec failed");
                exit(1);
            }
            /* parent process */
            default: {
                if (backgroundFlag == 0) {
                    if (waitpid(frkRtnVal, NULL, 0) == -1) perror("waitpid failed");
                } else {
                    /* store backgroundFlag job info */
                    if (job_count < MAX_JOBS) {
                        jobs[job_count].job_id = job_id_counter++;
                        jobs[job_count].pid = frkRtnVal;
                        strcpy(jobs[job_count].command, line_copy);
                        job_count++;
                        fprintf(stdout, "[%d] %d\n", jobs[job_count - 1].job_id, frkRtnVal);
                        fflush(stdout);
                    } else {
                        fprintf(stdout, "Too many backgroundFlag jobs\n");
                        fflush(stdout);
                    }
                }
                break;
            }
        }

    }/* while */
}/* main */
