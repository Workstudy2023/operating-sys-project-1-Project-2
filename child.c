#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
/*
The worker will be attached to shared memory and examine the simulated system clock. It will then figure out what time it
should terminate by adding up the system clock time and the time passed to it. This is when the process should decide to leave
the system and terminate.

For example, if the system clock was showing 6 seconds and 100 nanoseconds and the worker was passed 5 and 500000 as
above, the target time to terminate in the system would be 11 seconds and 500100 nanoseconds. The worker will then go into
a loop, constantly checking the system clock to see if this time has passed. If it ever looks at the system clock and sees values
over the ones when it should terminate, it should output some information and then terminate.
*/

#define SHMKEY 859047 // Parent and child agree on common key. Parent must create the shared memory segment.
// Size of shared memory buffer: two integers; one for seconds and the other for nanoeconds.
#define BUFF_SZ 2 * sizeof ( int ) 

int main(int argc, char** argv) {
		printf("Hello from Child.c, a new executable!\n");
		printf("My process id is: %d\n", getpid());
		printf(" I got %d arguments: \n", argc);

		// Process arguments
		int seconds = atoi(argv[1]);
		int nanoseconds = atoi(argv[2]);

		// Attach to shared memory
		// Examine simulated system clock
		int shmid = shmget(SHMKEY, BUFF_SZ, 0644); // Parent must create the shared memory segment. Read-only shared memory.
		if (shmid == -1)
		{
			fprintf(stderr, "Child: ... Error in shmget ...\n");
			exit(1);
		}
		int *cint = (int *)(shmat(shmid, 0, 0));
		if (cint == (int *)(-1))
		{
			fprintf(stderr, "Child: ... Error in shmat ...\n");
			exit(1);
		}

		// TODO: implement the signal handler

		// Figure out what time it should terminate by adding up the system clock time and the time
		// passed to it. This is when the process should decide to leave the system and terminate.
		// Get current time first.
		// Then add the seconds and nanoseconds to it.
		// Then check if the current time is greater than the target time.
		// If it is, then terminate.
		// If it is not, then keep looping.

		// Get simulated clock time
		int simulated_clock_seconds = cint[0];
		int simulated_clock_nanoseconds = cint[1];

		// Get target time
		int target_time_seconds = simulated_clock_seconds + seconds;
		int target_time_nanoseconds = simulated_clock_nanoseconds + nanoseconds;

		// TODO: implement the output
		// Check if target time is greater than simulated clock time
		while (1) {
			// Get simulated clock time
			simulated_clock_seconds = cint[0];
			simulated_clock_nanoseconds = cint[1];

			// Check if target time is greater than simulated clock time
			if (target_time_seconds < simulated_clock_seconds) {
				printf("Child: Target time is less than simulated clock time. Terminating.\n");
				break;
			}
			else if (target_time_seconds == simulated_clock_seconds) {
				if (target_time_nanoseconds < simulated_clock_nanoseconds) {
					printf("Child: Target time is less than simulated clock time. Terminating.\n");
					break;
				}
			}
		}

		shmdt(cint); // Detach from shared memory
		return EXIT_SUCCESS;
}
