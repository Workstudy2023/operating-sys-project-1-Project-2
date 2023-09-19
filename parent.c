#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void print_help()
{
    printf("Usage: oss [-h] [-n proc] [-s simul] [-t timelimit]\n");
    printf("  -h: Display this help message\n");
    printf("  -n proc: Number of total children to launch\n");
    printf("  -s simul: Number of children allowed to run simultaneously\n");
    printf("  -t timelimit: The bound of time that a child process will be launched for\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int total_children = 0;
    int max_simultaneous = 0;
    int time_limit = 0;

    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            return 0;
        case 'n':
            total_children = atoi(optarg);
            break;
        case 's':
            max_simultaneous = atoi(optarg);
            break;
        case 't':
            time_limit = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Invalid option: -%c\n", opt);
            print_help();
            return 1;
        }
    }

    if (total_children <= 0 || max_simultaneous <= 0 || time_limit <= 0) {
        fprintf(stderr, "Invalid parameters. Please provide valid values for -n, -s, and -t.\n");
        print_help();
        return 1;
    }

    // Keep track of the number of children currently running.
    int running_children = 0;

    pid_t childPid = fork(); // This is where the child process splits from the parent
    if (childPid == 0)
    {
        printf("I am a child but a copy of parent! My parent's PID is %d, and my PID is %d\n",
               getppid(), getpid());
        char *args[] = {"./child", "Hello",
                        "there", "exec", "is", "neat", 0};
        // execvp(args[0], args);

        execlp(args[0], args[0], args[1], args[2], args[3], args[4], args[5], args[6]);

        fprintf(stderr, "Exec failed, terminating\n");
        exit(1);
    }
    else
    {
        printf("I'm a parent! My pid is %d, and my child's pid is %d \n",
               getpid(), childPid);
        // sleep(1);
        wait(0);
    }
    printf("Parent is now ending.\n");
    return EXIT_SUCCESS;
}
