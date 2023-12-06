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

// Globals
int simClock[2] = {0, 0};    
int terminationTime[2] = {0, 0};    
int workerEndTime[2] = {0, 0}; 

#define SHM_KEY 205431 
#define PERMS 0644   

int message_amount = 0;

// Function prototypes
void getSimulatedClock();
void workerScheduler();

int main(int argc, char** argv)
{
    if (argc != 3) 
    {
        perror("Invalid command line arguments'\n");
        exit(1);
    }

    // Calculate worker end time
    workerEndTime[0] = atoi(argv[1]);
    workerEndTime[1] = atoi(argv[2]);

    key_t msgQueueKey = ftok("msgq.txt", 1);
    if (msgQueueKey == -1) 
    {
        perror("Failed to generate a key using ftok.\n");
        exit(1);
    }

    // get first clock time
    getSimulatedClock();

    // update worker termination time
    terminationTime[0] = simClock[0] + workerEndTime[0];
    terminationTime[1] = simClock[1] + workerEndTime[1];

    // start scheduler
    workerScheduler();

    return 0;
}

// Function to get current simulated clock time in system
void getSimulatedClock() 
{
    // Access and attach to shared memory to update the local simulated clock
    int sharedMemID = shmget(SHM_KEY, sizeof(int) * 2, 0777); 
    if (sharedMemID == -1) 
    {
        perror("Error: Failed to access shared memory using shmget.\n");
        exit(EXIT_FAILURE);
    }

    int* sharedMemPtr = (int*)shmat(sharedMemID, NULL, SHM_RDONLY);
    if (sharedMemPtr == NULL) 
    {
        perror("Error: Failed to attach to shared memory using shmat.\n");
        exit(EXIT_FAILURE);
    }

    simClock[0] = sharedMemPtr[0];     
    simClock[1] = sharedMemPtr[1];  
    shmdt(sharedMemPtr);
}

// Function to start worker schedule timer
void workerScheduler() 
{
    // Show first message
    if (message_amount == 0) 
    {
        printf("WORKER PID: %d, PPID: %d, SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d -- %s\n", 
            getpid(), getppid(), simClock[0], simClock[1], terminationTime[0], terminationTime[1], "Just Starting");

        message_amount += 1;
    }

    // Amount of seconds passed in the system
    int secondsPassed = 0;
    int currentSeconds = 0;

    // Check if target time is greater than simulated clock time
    while (simClock[0] < terminationTime[0]) 
    {
        // Get simulated clock time
        getSimulatedClock();
        
        // Check if a second has passed in the system
        if (simClock[0] > currentSeconds) 
        {
            currentSeconds = simClock[0];
            secondsPassed += 1;

            printf("WORKER PID: %d, PPID: %d, SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d -- %d %s \n", 
                getpid(), getppid(), simClock[0], simClock[1], terminationTime[0], terminationTime[1], 
                    secondsPassed, "seconds have passed since starting");
        }
    }

    // show termination msg
    printf("WORKER PID: %d, PPID: %d, SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d -- %s\n", 
        getpid(), getppid(), simClock[0], simClock[1], terminationTime[0], terminationTime[1], "Terminating");
}
