#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Constants
// Define a constant to limit the number of concurrent processes
#define MAX_CONCURRENT 20
#define BILLION 1000000000
#define TIME_STRING_SIZE 30

// Define a struct to represent a process control block (PCB)
struct PCB
{
    int occupied;     // either true or false
    pid_t pid;        // process id of this child
    int startSeconds; // time when it was forked
    int startNano;    // time when it was forked
};
struct PCB processTable[MAX_CONCURRENT];

/*
Each iteration in oss you need to increment the clock. So how much should you increment it? You should attempt to very
loosely have your internal clock be similar to the real clock. This does not have to be precise and does not need to be checked,
just use it as a crude guideline. So if you notice that your internal clock is much slower than real time, increase your increment.
If it is moving much faster, decrease your increment. Keep in mind that this will change based on server load possibly, so do
not worry about if it is off sometimes and on other times.
*/
// Our internal clock, similar to the real clock.
struct Clock *clock;
struct Clock
{
    int seconds;
    int nanoSeconds; // 1 billion nanoSeconds (1,000,000,000 nano seconds) = 1 second
};
#define SHMKEY 859047 // Parent and child agree on common key. Parent must create the shared memory segment.
// Size of shared memory buffer: two integers; one for seconds and the other for nanoeconds.
#define BUFF_SZ 2 * sizeof ( int ) 

// Function prototypes
void print_help();
void incrementClock();
void initializeClock();
int checkIfChildHasTerminated();
pid_t CreateChildProcess(int entryIndex, int time_limit);
int FindEntryInProcessTableNotOccupied();


int main(int argc, char *argv[])
{
    int opt;
    int total_children = 0;
    int max_simultaneous = 0;
    int time_limit = 0;

    // TODO: Signal handler for SIGINT

    // Create shared memory segment
    int shmid;
    // The permisssion mode should be read only for the child and read and write for the parent.
    // IPC_CREAT creates the shared memory segment if it does not already exist.
    // IPC_EXCL ensures that the shared memory segment is created for the first time.
    // 0777 is the permission mode. It is the same as 777 in octal and 511 in decimal. It means that the owner, group, and others have read, write, and execute permissions.
    if ((shmid = shmget(SHMKEY, BUFF_SZ, 0777 | IPC_CREAT | IPC_EXCL)) == -1) // TODO: check if this is correct
    {
        perror("shmget");
        exit(1);
    }
    if (shmid == -1)
    {
        fprintf(stderr, "Parent: ... Error in shmget ...\n");
        exit(1);
    }
    // Get the pointer to shared block of memory.
    int *pint = (int *)(shmat(shmid, 0, 0));
    clock = pint;
    pint[0] = 10; /* Write into the 'seconds' shared area. */
    pint[1] = 20; /* Write into the 'nanoseconds' shared area. */


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

    if (total_children <= 0 || max_simultaneous <= 0 || max_simultaneous > MAX_CONCURRENT || time_limit <= 0)
    {
        fprintf(stderr, "Invalid parameters. Please provide valid values for -n, -s, and -t.\n");
        print_help();
        return 1;
    }

    // Keep track of the number of children currently running.
    int running_children = 0;
    int entryIndex = 0;
    initializeClock();

    // Generate the children
    // Check if the number of children currently running is less than the maximum number of children allowed to run simultaneously.
    while (running_children < total_children)
    {
        incrementClock();
        // Every half a second of simulated clock time, output the process table to the screen
        if (clock->nanoSeconds >= 500000000) // if nanoSeconds is greater than or equal a half second
            printProcessTable();

        // Find entry in process table that is not occupied
        int entryIndex = FindEntryInProcessTableNotOccupied();
        if (entryIndex == -1)
        {
            printf("No entry in process table is not occupied.\n");
            continue;
        }

        pid_t childPid = CreateChildProcess(entryIndex, time_limit); // This is where the child process splits from the parent
        if (childPid != 0)                                           // parent process
        {
            printf("I'm a parent! My pid is %d, and my child's pid is %d \n", getpid(), childPid);
        }

        // Check if any children have terminated
        int childHasTerminated = checkIfChildHasTerminated();
        if (childHasTerminated)
        {
            // update PCB Of the Terminated Child
            updatePCB(childHasTerminated);
            running_children--;
            // Launch new child obeying process limits
            pid_t newChildPid = CreateChildProcess(entryIndex, time_limit); // This is where the child process splits from the parent
            if (childPid != 0)                                              // parent process
            {
                printf("I'm a parent! My pid is %d, and my child's pid is %d \n", getpid(), childPid);
            }
        }
        running_children++;
    }
    printf("Parent is now ending.\n");
    // Detach and remove shared memory segment
    // TODO: Detach the shared memory segment ONLY if no children are active.
    if (shmdt(pint) == -1) 
    {
        fprintf(stderr, "Parent: ... Error in shmdt ...\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) 
    {
        fprintf(stderr, "Parent: ... Error in shmctl ...\n");
        exit(1);
    }

    return EXIT_SUCCESS;
}


// Function definitions
void print_help()
{
    printf("Usage: oss [-h] [-n proc] [-s simul] [-t timelimit]\n");
    printf("  -h: Display this help message\n");
    printf("  -n proc: Number of total children to launch\n");
    printf("  -s simul: Number of children allowed to run simultaneously\n");
    printf("  -t timelimit: The bound of time that a child process will be launched for\n");
}

/*
This function should be called the time right before oss does a fork to
launch that child process (based on our own simulated clock).
*/
void CreatePCBentry(int entryIndex, pid_t pid, int startSeconds, int startNano)
{
    processTable[entryIndex].occupied = 1;
    processTable[entryIndex].pid = pid;
    processTable[entryIndex].startSeconds = startSeconds;
    processTable[entryIndex].startNano = startNano;
}

/*
This function updates the PCB of a terminated child. It should set the occupied flag to 0 (false),
set the pid, startSeconds, and startNano to 0.
childHasTerminated represents the pid of the child that has terminated.
*/
void updatePCB(int pidChildHasTerminated)
{
    for (int i = 0; i < MAX_CONCURRENT; i++)
    {
        if (processTable[i].pid == pidChildHasTerminated)
        {
            processTable[i].occupied = 0;
            processTable[i].pid = 0;
            processTable[i].startSeconds = 0;
            processTable[i].startNano = 0;
        }
    }
}

/*
The output of oss should consist of, every half a second in our simulated system, outputting the entire process table in a nice
format. For example:
OSS PID:6576 SysClockS: 7 SysclockNano: 500000
Process Table:
Entry Occupied PID StartS StartN
0 1 6577 5 500000
1 0 0 0 0
2 0 0 0 0
...
19 0 0 0 0
*/
void printProcessTable()
{
    printf("OSS PID: %d SysClockS: %d SysclockNano: %d\n", getppid(), clock->seconds, clock->nanoSeconds);
    printf("Process Table:\n");
    printf("Entry Occupied PID StartS StartN\n");
    for (int i = 0; i < MAX_CONCURRENT; i++)
    {
        printf("%d %d %d %d %d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
    }
}

void incrementClock()
{
    clock->nanoSeconds += 1000;           // increment by 1000 nanoSeconds (1 microsecond); this is the increment referred to in the comment above
    if (clock->nanoSeconds >= 1000000000) // if nanoSeconds is greater than or equal a second
    {
        clock->seconds++;
        clock->nanoSeconds = 0;
    }
}

// initialize the clock
void initializeClock()
{
    clock->seconds = 0;
    clock->nanoSeconds = 0;
}

/*
The check to see if a child has terminated should be done with a nonblocking wait() call. This can be
done with code along the lines of:

int pid = waitpid(-1, &status, WNOHANG);

waitpid will return 0 if no child processes have terminated and will return the pid of the child if
one has terminated.
*/
int checkIfChildHasTerminated()
{
    int status;
    int pid = waitpid(-1, &status, WNOHANG);
    if (pid == 0) // no child processes have terminated
    {
        return 0;
    }
    else // a child has terminated
    {
        return pid; // TODO: return the pid of the child that has terminated
    }
}

pid_t CreateChildProcess(int entryIndex, int time_limit)
{
    int randomSeconds = rand() % time_limit + 1;
    int randomNanoSeconds = rand() % BILLION;
    pid_t childPid = fork(); // This is where the child process splits from the parent
    if (childPid == 0)       // child process
    {
        printf("I am a child but a copy of parent! My parent's PID is %d, and my PID is %d\n", getppid(), getpid()); // TODO: remove this line
        // the -t parameter is different. It now stands for the bound of time that a child process will be launched for.
        // So for example, if it is called with -t 7, then when calling worker processes, it should call them with a
        // time interval randomly between 1 second and 7 seconds (with nanoseconds also random).
        // Convert randomSeconds and randomNanoSeconds to strings
        char randomSecondsString[TIME_STRING_SIZE];
        char randomNanoSecondsString[TIME_STRING_SIZE];
        sprintf(randomSecondsString, "%d", randomSeconds);
        sprintf(randomNanoSecondsString, "%d", randomNanoSeconds);
        char *args[] = {"./worker", randomSecondsString, randomNanoSecondsString, 0};
        execlp(args[0], args[1], args[2], args[3]); // check if this works with double args[0]
        exit(EXIT_SUCCESS);
    }
    CreatePCBentry(entryIndex, childPid, randomSeconds, randomNanoSeconds);
    return childPid;
}

/*
Find entry in process table that is not occupied
Returns the index of the entry in the process table that is not occupied.
If no entry is found, returns -1.
*/
int FindEntryInProcessTableNotOccupied()
{
    int entryIndex = -1;
    for(int i = 0; i < MAX_CONCURRENT; i++)
    {
        if (processTable[i].occupied == 0)
        {
            entryIndex = i;
            break;
        }
    }
    return entryIndex;
}
