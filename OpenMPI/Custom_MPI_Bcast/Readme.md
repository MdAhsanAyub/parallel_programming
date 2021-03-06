# Project Title
Comparison and Implementation of Custom MPI_Bcast 

### Description
This is a sample program to implement a custom MPI broadcast. The custom drive program didn't use any other MPI group communication routine other than MPI point-to-point communication, e.g. `MPI_Send` and `MPI_Recv`. However, the arguments of the custom broadcast routine is same as `MPI_Bcast`. The program has been tested with an array of double with 100,000 elements and then compared with the default routine.

### Prerequisites

- OpenMPI Library
- GNU Library

### Execution

A step by step series of examples that tell you how to get a development env running

```
Step 1. Transfer the cpp files and Makefile in the HPC cluster
Step 2. command: make coompile
Step 3. command: make run
Step 4. command: make clean
```

### Example
```.. code-block:: console
	$ hpcshell --ntasks-per-node=4
	$ make compile
	mpic++ -o main collective_communication.cpp maa_bcast.cpp
	$ make run
	mpirun -np 4 ./main
	....
	....
	//A lot of text
	$ make clean
	rm main
```
