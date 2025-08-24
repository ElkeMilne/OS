/*********************************************************************
   Program  : miniShell                   Version    : 1.6
 --------------------------------------------------------------------
   Fully functional Linux/Unix command line interpreter
   Fixes cd command, background jobs, immediate job reporting,
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
#define MAX_JOBS 100    /* maximum number of background jobs */

char line[NL];          /* command input buffer */

/* structure to store background job info */
typedef struct {
    int job_id;
    pid_t pid;
    char command[NL];
} Job;

Job jobs[MAX_JOBS];             /* active background jobs */
int job_count = 0;              /* number of active jobs */
int job_id_counter = 1;         /* auto-increment job ID */

/*
    shell prompt
*/
void prompt(void)
{
    // ## REMOVE THIS 'fprintf' STATEMENT BEFORE SUBMISSION
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

/*
    signal handler for SIGCHLD
    reports immediately when a background process finishes
*/
void sighandler(int sig)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                /* print finished job immediately */
                if (WIFEXITED(status)) {
                    fprintf(stdout, "\n[%d]+ Done %s (exit=%d)\n",
                            jobs[i].job_id, jobs[i].command, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    fprintf(stdout, "\n[%d]+ Terminated %s (signal=%d)\n",
                            jobs[i].job_id, jobs[i].command, WTERMSIG(status));
                }
                fflush(stdout);

                /* remove job from active job list */
                for (int j = i; j < job_count - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
                break;
            }
        }
    }
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
    int frkRtnVal;              /* value returned by fork sys call */
    char *v[NV];                /* array of pointers to command line tokens */
    const char *sep = " \t\n";  /* command line token separators */
    int i;                      /* parse index */
    int backgroundFlag;          /* background flag */

    /* install SIGCHLD handler to track finished background jobs */
    if (signal(SIGCHLD, sighandler) == SIG_ERR) {
        perror("signal failed");
        exit(1);
    }

    /* prompt for and process one command line at a time */
    while (1) {                  /* do forever */
        prompt();

        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                exit(0);
            }
            perror("fgets failed");
            continue;
        }

        /* ignore blank lines or comments */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;          /* back to prompt */
        }

        /* copy line for job tracking */
        char line_copy[NL];
        strcpy(line_copy, line);

        /* tokenize input line */
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }
        v[i] = NULL;          /* ensure NULL-terminated */
        if (v[0] == NULL) continue;

        /* check for background '&' */
        backgroundFlag = 0;
        if (i > 1 && strcmp(v[i - 1], "&") == 0) {
            backgroundFlag = 1;
            v[i - 1] = NULL;
            line_copy[strcspn(line_copy, "&")] = 0; /* remove '&' from command copy */
        }

        /* handle built-in cd command */
        if (strcmp(v[0], "cd") == 0) {
            int ret;
            if (v[1] == NULL) {
                ret = chdir(getenv("HOME"));   /* default HOME */
            } else {
                v[1][strcspn(v[1], "\n")] = 0; /* remove possible trailing newline */
                ret = chdir(v[1]);
            }
            if (ret != 0) {  /* check for errors */
                perror("cd failed");
            }
            continue;  /* back to prompt */
        }

        /* fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
            case -1: {          /* fork error */
                perror("fork failed");
                break;
            }
            case 0: {           /* child process */
                execvp(v[0], v);
                perror("exec failed");  /* exec failed */
                exit(1);              /* terminate child */
            }
            default: {          /* parent process */
                if (backgroundFlag == 0) {
                    if (waitpid(frkRtnVal, NULL, 0) == -1) {
                        perror("waitpid failed");
                    }
                } else {
                    /* store background job */
                    if (job_count < MAX_JOBS) {
                        jobs[job_count].job_id = job_id_counter++;
                        jobs[job_count].pid = frkRtnVal;
                        strcpy(jobs[job_count].command, line_copy);
                        job_count++;
                        fprintf(stdout, "[%d] %d\n", jobs[job_count - 1].job_id, frkRtnVal);
                        fflush(stdout);
                    } else {
                        fprintf(stdout, "Too many background jobs\n");
                        fflush(stdout);
                    }
                }
                break;
            }
        } /* switch */
    } /* while */
} /* main */
