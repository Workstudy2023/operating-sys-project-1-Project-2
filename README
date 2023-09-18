
# Operating System Project:  OSS and Worker Processes
This is a simple project to understand the basics of managing child processes in Linux using C and creating a Makefile. The project consists of two executables: ‘oss’ (the parent process) and ‘worker’ (the child process).
* Table of Contents
* Getting Started 
* Usage 
* Implementaion Details
* Makefile
* Project Structure 
* Contributor

 Project Overview
 ‘worker’ Process
The worker process takes a single command-line argument specifying the number of iterations it should perform. For example:
./worker 5

The worker process will then execute a loop for the specified number of iterations. In each iteration, it will output its PID, its parent's PID, and the current iteration number before and after sleeping for one second.
# oss Process (Parent)

The oss process is responsible for launching multiple worker processes with specific parameters. The parameters for oss are as follows:
oss [-h] [-n proc] [-s simul] [-t iter]

-h: Display a help message.
-n proc: Number of total children to launch.
-s simul: Number of children allowed to run simultaneously.
-t iter: Number of iterations for each worker.
For example, to launch ‘oss’ with 5 ‘worker’ processes, allowing a maximum of 3 to run simultaneously, and each ‘worker’ performing 7 iterations:
oss -n 5 -s 3 -t 7
‘oss’ will create child processes and manage their execution.
# Getting Started
1.	Clone this repository to your local machine:
git clone <repository_url>

2. Navigate to the project directory:
cd <project_directory>

3.	Compile the project using the provided Makefile (see Makefile).
4.	Run the ‘oss’ process with the desired command-line parameters (see Usage).

# Usage
To compile and run the project, follow these steps:
1.	Compile the project by running:
make
Run the oss process with the desired parameters. For example:

./oss -n 5 -s 3 -t 7
* 	-n: Number of total children to launch.
* 	-s: Number of children allowed to run simultaneously.
* 	-t: Number of iterations for each worker.
*  The ‘oss’ process will manage the ‘worker’ processes, and you will see the output of the ‘worker’ processes' iterations.
*   To terminate the program, press Ctrl + C.

# Implementation Details
* 	The oss process manages child processes (worker) and ensures that the maximum number of concurrent child processes (-s simul) is not exceeded.
* 	Each worker process takes a command-line argument (-t iter) specifying the number of iterations it should perform.
* 	The parent process (oss) waits for child processes to finish before launching new ones.
The getopt library is used to parse command-line arguments.

# Makefile
The provided Makefile simplifies the compilation process. To compile both ‘oss’ and ‘worker’, run:

*  Make

To clean up the generated files, run:
* make clean

# Project Structure
* 	oss.c: Contains the implementation of the oss process.
* 	worker.c: Contains the implementation of the worker process.
* 	Makefile: Defines compilation rules for the project.
* 	README: This README file.

# One Line Example of how to run the project:
./oss -n 5 -s 3 -t 7
* This command will run the ‘oss’ process, launching 5 ‘worker’ processes, allowing a maximum of 3 to run simultaneously, with each ‘worker’ performing 7 iterations.
# Contibutor 
* Christine Mckelvey

