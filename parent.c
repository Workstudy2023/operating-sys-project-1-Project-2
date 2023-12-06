#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define SHM_KEY 205431 
#define PERMS 0644   

// Define a struct to represent a process control block (PCB)
typedef struct PCB {
    pid_t pid;        // process id of this child
    int occupied;     // either true or false
    int startSeconds; // time when it was forked
    int startNano;    // time when it was forked
} pcb;

// initialize with max size processes
pcb processTable[20]; 
 
unsigned int shmID;             
unsigned int* shmPtr;   
unsigned int simClock[2] = {0, 0}; // simulated clock

int processCount = 0;
int processTimeLimit = 0;
int simultaneousCount = 0;

// process launching variables
int totalLaunched = 0;
int totalTerminated = 0;
unsigned long long launchTimePassed = 0;

int halfsecond = 500000000;
int halfSecondPassed = 0;
long lastClockTime = 0;

// function prototypes
void handleTermination();
void print_help();
void print_table();
void launchworkers();
void incrementSimulatedClock();
void checkTerminatedProcesses();

// main
int main(int argc, char** argv) {
    // add randomness
    srand(time(NULL) + getpid());

    // Register signal handlers for interruption and timeout
    signal(SIGINT, handleTermination);
    signal(SIGALRM, handleTermination);
    alarm(60); 
      
    // Iterate through arguments
    int argument;
    while ((argument = getopt(argc, argv, "n:s:t:h")) != -1) 
    {
        switch (argument)
        {
            case 'n':
                processCount = atoi(optarg);
                if (processCount <= 0) 
                {
                    perror("invalid process count provided\n");
                    exit(1);
                }
                break;
            case 's':
                simultaneousCount = atoi(optarg);
                if (simultaneousCount > 18)
                {   
                    perror("invalid simulataneous amount provided\n");
                    exit(1);
                }
                break;
            case 't':
                processTimeLimit = atoi(optarg);
                if (processTimeLimit <= 0) 
                {
                    perror("invalid time limit provided\n");
                    exit(1);
                }
                break;
            case 'h':
                print_help();
                exit(0);
            default:
                printf("%s", "invalid argument provided\n");
                exit(1);
        }
    }

    // check if arguments are all correct
    if (processCount <= 0 || simultaneousCount <= 0 || processTimeLimit <= 0) 
    {
        perror("invalid argument(s) values provided\n");
        exit(1);
    }

    // Set up memory and initialize the process table
    for (int i = 0; i < processCount; i++) 
    {
        processTable[i].pid = 0;
        processTable[i].occupied = 0;
        processTable[i].startNano = 0;
        processTable[i].startSeconds = 0;
    }

    // Get a shared memory segment
    shmID = shmget(SHM_KEY, sizeof(unsigned int) * 2, 0777 | IPC_CREAT);
    if (shmID == -1) 
    {
        perror("Unable to acquire the shared memory segment.\n");
        exit(1);
    }
    
    shmPtr = (int*)shmat(shmID, NULL, 0);
    if (shmPtr == NULL) 
    {
        perror("Unable to connect to the shared memory segment.\n");
        exit(1);
    }

    lastClockTime = clock();
    memcpy(shmPtr, simClock, sizeof(unsigned int) * 2);
    launchworkers();
    handleTermination();
    return 0;
}

// Function to start worker scheduling
void launchworkers() {
    // continue until children terminate
    while (totalTerminated != processCount) 
    {
        // update simulated clock
        incrementSimulatedClock();

        // determine if we should launch a child
        if (totalLaunched < processCount && totalLaunched - totalTerminated < simultaneousCount) 
        {
            // launch new child
            pid_t pid = fork();
            if (pid == 0) {
                srand(time(NULL) + getpid());
                int randomSeconds = rand() % processTimeLimit + 1;
                int randomNanoSeconds = rand() % 100000000 + 1;

                char randomSecondsString[50];
                char randomNanoSecondsString[50];
                sprintf(randomSecondsString, "%d", randomSeconds);
                sprintf(randomNanoSecondsString, "%d", randomNanoSeconds);

                char* args[] = {"./worker", randomSecondsString, randomNanoSecondsString, NULL};
                execvp(args[0], args);
            } 
            else {
                processTable[totalLaunched].pid = pid;
                processTable[totalLaunched].occupied = 1;
                processTable[totalLaunched].startNano = simClock[1];
                processTable[totalLaunched].startSeconds = simClock[0];
            }

            totalLaunched += 1;
        }

        // display table every half a second
        if (simClock[1] - halfSecondPassed >= halfsecond) 
        {
            print_table();
            halfSecondPassed = simClock[1];
        }

        // check for terminated processes
        checkTerminatedProcesses();
    }

    printf("\nOSS: finished\n");
}

// function to see if processes terminated
void checkTerminatedProcesses() {
    // check occupied processes
    for (int i=0; i<totalLaunched; i++) 
    {
        if (processTable[i].occupied == 1)
        {
            int childStatus;
            pid_t childPid = processTable[i].pid;
            pid_t result = waitpid(childPid, &childStatus, WNOHANG);

            if (result > 0) 
            {
                processTable[i].occupied = 0;
                totalTerminated += 1;
            }
        }
    }
}

// function to print process table
void print_table() {
    // Print to the screen
    printf("\nOSS PID: %d SysClockS: %d SysclockNano: %d\nProcess Table: \n%-6s%-10s%-8s%-12s%-12s\n",
            getpid(), simClock[0], simClock[1], "Entry", "Occupied", "PID", "StartS", "StartN");

    for (int i = 0; i < totalLaunched; i++) {
        printf("%-6d%-10d%-8d%-12u%-12u\n",
                i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
    }
    printf("\n");
}

// function to print help screen
void print_help()
{
    printf("\nUsage: oss [-h] [-n proc] [-s simul] [-t timelimit]\n");
    printf("  -h: Display this help message\n");
    printf("  -n proc: Number of total children to launch\n");
    printf("  -s simul: Number of children allowed to run simultaneously, 18 maximum\n");
    printf("  -t timelimit: The bound of time that a child process will be launched for\n\t example: -t 7: would be a random time between 1 and 7 for seconds, with random nanoseconds\n\n");
}

// Function to update the clock
void incrementSimulatedClock() {
    // update clock, mimic real life time
    clock_t realLifeTime = clock();
    double secondsPassed = ((double)(realLifeTime - lastClockTime)) / CLOCKS_PER_SEC;
    int nanoSecondsPassed = (int)(secondsPassed * 1e9);

    // determine new seconds and nanoseconds 
    // simulated clock time and update shared memory
    simClock[0] += secondsPassed;
    simClock[1] += nanoSecondsPassed;

    if (simClock[1] >= 1000000000) {
        simClock[0] += 1;
        simClock[1] -= 1000000000;
    }

    memcpy(shmPtr, simClock, sizeof(unsigned int) * 2);
    lastClockTime = realLifeTime;
}

// function to handle terminating program
void handleTermination() {
    printf("\n\nOSS: terminating and cleaning up program\n");
    shmdt(shmPtr);
    shmctl(shmID, IPC_RMID, NULL); 
    kill(0, SIGTERM);  
}

