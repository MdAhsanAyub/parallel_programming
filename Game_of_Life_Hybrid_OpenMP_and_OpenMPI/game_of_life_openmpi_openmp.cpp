/*
 *
 * The Game of Life
 *		- OpenMPI /w OpenMP implementation | A hybrid approach
 *
 * Rules of the game: Adapted from - http://codingdojo.org/kata/GameOfLife/ 
 *
 * Any live cell with fewer than two live neighbours dies, as if caused by underpopulation.
 * Any live cell with more than three live neighbours dies, as if by overcrowding.
 * Any live cell with two or three live neighbours lives on to the next generation.
 * Any dead cell with exactly three live neighbours becomes a live cell.
 *
 * @author Md. Ahsan Ayub
 * @version 3.4 04/17/2019 
 *
 */

#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

using namespace std;

// Declaring global grid array
int **iGrid;
int **iGridNew;

// Actial values will be initialized after knowing the user defined grid dimension
int iActualRowCount = 0, iActualColumnCount = 0;

// Allocate the arrays
void allocateGrids(int iRowSize, int iColumnSize)
{
	iGrid = new int*[iRowSize];
	iGridNew = new int*[iRowSize]; // A copy of iGrid
	for (int i = 0; i < iRowSize; i++)
	{
		iGrid[i] = new int[iColumnSize];
		iGridNew[i] = new int[iColumnSize];
	}
}

// Add outer layers to the grid
void addOuterLayers()
{
	// Add top layer which is the copy of the iRowCount th stride row-wise.
	for(int i = 1; i <= (iActualColumnCount - 2); i++)
		iGrid[0][i] = iGrid[iActualRowCount - 2][i];

	// Add bottom layer which is the copy of the 1 st stride row-wise.
	for(int i = 1; i <= (iActualColumnCount - 2); i++)
		iGrid[iActualRowCount - 1][i] = iGrid[1][i];

	// Add left layer which is the copy of the iColumnCount st stride column-wise.
	for(int i = 1; i <= (iActualRowCount - 2); i++)
		iGrid[i][0] = iGrid[i][iActualColumnCount - 2];

	// Add right layer which is the copy of the 1 st stride column-wise.
	for(int i = 1; i <= (iActualRowCount - 2); i++)
		iGrid[i][iActualColumnCount-1] = iGrid[i][1];

	// Diagonal copies
	iGrid[0][0] = iGrid[iActualRowCount - 2][iActualColumnCount - 2];	// Upper Left
	iGrid[0][iActualColumnCount - 1] = iGrid[iActualRowCount - 2][1];	// Upper Right
	iGrid[iActualRowCount - 1][0] = iGrid[1][iActualColumnCount - 2];	// Lower Left
	iGrid[iActualRowCount - 1][iActualColumnCount - 1] = iGrid[1][1];	// Lower Right
}

// Print routine of iGrid
void printGrid(int iRowCountStart, int iRowCountEnd, int iColumnCountStart, int iColumnCountEnd)
{
	for(int i = iRowCountStart; i < iRowCountEnd; i++)
	{
		for(int j = iColumnCountStart; j < iColumnCountEnd; j++)
			cout << iGrid[i][j] << " ";printf("%s\n", );
		cout << endl;
	}
}

// Copy routine to iGrid from iGridNew (to make a new state's copy)
void copyGrid(int iRowCount, int iColumnCount)
{
	for(int i = 0; i < iRowCount; i++)
		for(int j = 0; j < iColumnCount; j++)
			iGrid[i][j] = iGridNew[i][j];
}

// Measure the neighbors' status of a particular cell
int adjacentNeighbors(int i, int j)
{
	// Variables to calculate the number of alive and dead cells
	int iAliveCount = 0, iDeadCount = 0;

	iGrid[i - 1][j - 1] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i - 1][j] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i - 1][j + 1] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i][j + 1] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i + 1][j + 1] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i + 1][j] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i + 1][j - 1] == 0 ? iDeadCount++ : iAliveCount++;
	iGrid[i][j - 1] == 0 ? iDeadCount++ : iAliveCount++;

	if(iAliveCount < 2)
		// A cell dies of loneliness
		return 0;

	else if(iAliveCount > 3)
		// A cell dies of overcrowding
		return 0;

	else if(iGrid[i][j] == 1 && (iAliveCount == 2 || iAliveCount == 3))
		// Current cell is alive, and there are TWO (which will become three in total) and exactly THREE cells alive.
		// Thus, the cell survives.s
		return 1;

	else if(iGrid[i][j] == 0 && iAliveCount == 3)
		// Current cell is NOT alive; however, there are exactly THREE alive cells: thus, the cell is born.
		return 1;
	
	else
		// Very unlikely this will be executed
		return 0;
}

// Add an outer layer of the whole array for the simplicity

// Main function
int main(int argc, char *argv[])
{
	// Checking the number of input has to be passed by the user
	if (argc < 3)
    {
        printf("Usuage: ./<executable> <input_file> <iterations> <output_file>\n");
        return -1;
    }

    // Getting values from the argument
    ifstream fInput(argv[1]);
    int thread_count = atoi(argv[2]);
    int iGenerations = atoi(argv[3]);
    ofstream fOutput(argv[4]);

    // Checking whether the input file exists in the directory or not
    if (!fInput)
	{
		printf("Error opening the input file.\n");
		return -1;
	}

	// Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); // Total number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank); // Rank of processes starting from 0 till (world_size - 1)

    // Initializing necessary variables
	int iRowCount, iColumnCount, iNeighborStatus, iChunkSize, iChunkRemainder, iStartRowIndex, iEndRowIndex;
	char cItem;

	// Execution time calculation variables
	double dStartTime, dEndTime;

	// Getting the grid dimension from the first line of the input file
	fInput >> iRowCount >> iColumnCount;
	iActualRowCount = iRowCount + 2; // Two new layers will be added: Top and Bottom
	iActualColumnCount = iColumnCount + 2; // Two new layers will be added: Left and 

	//cout << iRowCount << "\t" << iColumnCount << endl;

	// Imposing the condition: Size of the processes will not exceed the number of rows of the grid
    if(world_size > iRowCount)
    {
    	cout << "Defined Processes: " << world_size << " | Number of the Grid's Row: " << iRowCount << endl;
    	cout << "Size of the processes will not exceed the number of rows of the grid" << endl;
    	return -1;
    }

	iChunkSize =  iRowCount / world_size; // Calculating the size of the chunk against each processes
	iChunkRemainder = iRowCount % world_size; // Calculating the remainder to deal with the odd number of processes for later on.

	// Process 0 will initialize the grid and distribute the tasks to other processes
	if(world_rank == 0)
	{
		// Measure the starting clock time
        dStartTime = MPI_Wtime();

		// Allocate 2D arrays dynamically
		allocateGrids(iActualRowCount, iActualColumnCount);

		cout << "Allocating grid successfully" << endl;

		// Populating the array from file
		int iCounterRow = 1, iCounterColumn = 1; // Ignoring 0 indexes, as it will later be added as outer layers!
		while(fInput >> cItem)
		{
			// Condition to always maintain the correct index order of the 2D array
			if(iCounterColumn > iColumnCount)
			{
				iCounterRow++;
				iCounterColumn = 1;
				//cout << endl;
			}

			if((iCounterColumn > iColumnCount) || (iCounterRow > iRowCount))
				continue;

			//cout << cItem << "\n";
			iGrid[iCounterRow][iCounterColumn] = cItem - '0';
			//cout << iGrid[iCounterRow][iCounterColumn] << endl;
			
			//cout << "Row: " << iCounterRow << " , Column: " << iCounterColumn << endl;
			iCounterColumn++;
		}
		cout << "Row: " << iCounterRow << " , Column: " << iCounterColumn << endl;

		cout << "Exit from file input buffering" << endl;

		// As the file has read properly, now it's time to add outer layers
		addOuterLayers();

		cout << "File input completed.. " << endl;
		//printGrid(1, iActualRowCount - 1, 1, iActualColumnCount - 1);
		cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

		//cout << thread_count << " " << world_size << endl;

		// Calculating the starting and end index for processes 0
		iStartRowIndex = ((0 * iChunkSize) + 1);
		iEndRowIndex = iStartRowIndex + iChunkSize;

		// Adding outer layer into consideration
		iStartRowIndex -= 1;
		iEndRowIndex += 1;


		for(int i = 1; i < world_size; i++) 
        {
        	int iSizeOfTheBuffer;

            // Last process will avail the chuck size and the remainder
            if(i == (world_size - 1))
            {
            	// Calculating the starting and end index for each processes
				iStartRowIndex = ((i * iChunkSize) + 1);
				if(iChunkRemainder)	// Last process will consider the remainder
					iChunkSize = iChunkSize + iChunkRemainder;
				iEndRowIndex = iStartRowIndex + iChunkSize;
				
				// Adding outer layer into consideration
				iStartRowIndex -= 1;
				iEndRowIndex += 1;

                // Initialzing the grid slice which needs to be passed via MPI Send
                int iSizeOfTheRow = iEndRowIndex - iStartRowIndex;
              
                // Calculate the buffer size
                iSizeOfTheBuffer = iSizeOfTheRow * iActualColumnCount;

                // Create a local copy
				int *iGridSlice = new int[iSizeOfTheBuffer];

                // Sending the size of the row
                MPI_Send(&iSizeOfTheRow, 1, MPI_INT, i, 1, MPI_COMM_WORLD);

                cout << "IF block for Process " << i << " | Buffer Size: " << iSizeOfTheBuffer << endl;
                cout << "Start Row Index: " << iStartRowIndex << " | End Row Index: " << iEndRowIndex << endl;

                for(int j = iStartRowIndex, k = 0; j < iEndRowIndex; j++, k++)
                {
                	for(int l = 0; l < iActualColumnCount; l++)
            		{
                 		iGridSlice[(k*iActualColumnCount)+l] = iGrid[j][l];
                 		//cout << iGridSlice[(k*iActualColumnCount)+l] << " ";
                  	}
                  	//cout << endl;
                }
                //cout << iGridSlice[iSizeOfTheBuffer-2] << endl;

                cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

                // Sending the buffer to the last process
                MPI_Send(iGridSlice, iSizeOfTheBuffer, MPI_INT, i, 1, MPI_COMM_WORLD);

                cout << "Task distribution send completed.." << endl;

                // free the allocated array
				delete[] iGridSlice;
			}            

            // Rest of the other processes (1 to n-1) will avail the chunk size partioning only.
            else 
            {
            	// Calculating the starting and end index for each processes
				iStartRowIndex = ((i * iChunkSize) + 1);
				iEndRowIndex = iStartRowIndex + iChunkSize;
				
				// Adding outer layer into consideration
				iStartRowIndex -= 1;
				iEndRowIndex += 1;

                // Initialzing the grid slice which needs to be passed via MPI Send
                int iSizeOfTheRow = iEndRowIndex - iStartRowIndex;
              
                // Calculate the buffer size
                iSizeOfTheBuffer = iSizeOfTheRow * iActualColumnCount;

                // Create a local copy
				int *iGridSlice = new int[iSizeOfTheBuffer];

                // Sending the size of the row
                MPI_Send(&iSizeOfTheRow, 1, MPI_INT, i, 1, MPI_COMM_WORLD);

                cout << "IF block for Process " << i << " | Buffer Size: " << iSizeOfTheBuffer << endl;
                cout << "Start Row Index: " << iStartRowIndex << " | End Row Index: " << iEndRowIndex << endl;

                for(int j = iStartRowIndex, k = 0; j < iEndRowIndex; j++, k++)
                {
                	for(int l = 0; l < iActualColumnCount; l++)
            		{
                 		iGridSlice[(k*iActualColumnCount)+l] = iGrid[j][l];
                 		//cout << iGridSlice[(k*iActualColumnCount)+l] << " ";
                  	}
                  	//cout << endl;
                }
                //cout << iGridSlice[iSizeOfTheBuffer-2] << endl;

                cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

                // Sending the buffer to the last process
                MPI_Send(iGridSlice, iSizeOfTheBuffer, MPI_INT, i, 1, MPI_COMM_WORLD);

                cout << "Task distribution send completed.." << endl;

                // free the allocated array
				delete[] iGridSlice;
			}
        }
	}
	else
	{
		// Let all the processes get synchronized
	    //MPI_Barrier(MPI_COMM_WORLD);

	    // Receiving the size of row first
	    MPI_Recv(&iRowCount, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	    //cout << "Process: " << world_rank << endl;

		// Calculate the buffer size (for each process)
	    int iSizeOfTheBuffer = iRowCount * iActualColumnCount;

	    // Create a local copy
		int *iGridLocal = new int[iSizeOfTheBuffer];
		
		// Let all the processes get synchronized
	    //MPI_Barrier(MPI_COMM_WORLD);

		// Time to receive the slice grids
	    MPI_Recv(iGridLocal, iSizeOfTheBuffer, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		cout << "Process: " << world_rank << " | Number of Rows: " << iRowCount << " | Number of Columns: " << iActualColumnCount << " | Buffer Size: " << iSizeOfTheBuffer << endl;
		
		// Now, create two grids for generations.
		allocateGrids(iRowCount, iActualColumnCount);

		// Let all the processes get synchronized
	    //MPI_Barrier(MPI_COMM_WORLD);

		// Assigning 1D array values to 2D above created array
		int iTempIndex = 0;
		for(int i = 0; i < iRowCount; i++)
		{
			for(int j = 0; j < iActualColumnCount; j++)
			{
				iGrid[i][j] = iGridNew[i][j] = iGridLocal[iTempIndex];
				iTempIndex += 1;
				//cout << iGrid[i][j] << " ";
			}
			//cout << endl;
		}

		//cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
	}

	if(world_rank == 0)
		iRowCount =  iEndRowIndex - iStartRowIndex;

	int iThreadChunk = iRowCount / thread_count;
	int iThreadChunkRemainder = iRowCount % thread_count;
	int iThreadStartIndex = -1, iThreadEndIndex = -1;

	// Let all the processes get synchronized
	//MPI_Barrier(MPI_COMM_WORLD);

	cout << "Process " << world_rank << endl;
	cout << "Row Index Starts: 1 | Row Index Ends: " << (iRowCount - 1) << " | Column Starts: 1 | Column Ends: " << (iActualColumnCount - 1) << endl;  

	/*for(int i = 1; i < (iRowCount - 1); i++)
	{
		for(int j = 1; j < (iActualColumnCount - 1); j++)
			cout << iGrid[i][j] << " ";
		cout << endl;
	}
	cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;*/

	// All set for the game
	for(int iSteps = 1; iSteps <= iGenerations; iSteps++)
	{
		// Let all the processes get synchronized
    	//MPI_Barrier(MPI_COMM_WORLD);
    	
    	//cout << "Process: " << world_rank << " | Generation: " << iSteps << endl;


		// geting into the OpenMP parallel region
		#pragma omp parallel firstprivate(iThreadStartIndex, iThreadEndIndex) num_threads(thread_count)
    	{
    		int iMyRank = omp_get_thread_num(); //What thread am I?
    		
    		// Computing two varibles to buffer through the grid which are derivatives from iMyRank variable
    		iThreadStartIndex = iMyRank * iThreadChunk;
    		if((iThreadStartIndex + iThreadChunk) > iRowCount)
    			iThreadEndIndex = iThreadChunkRemainder;
    		else
    			iThreadEndIndex = iThreadStartIndex + iThreadChunk;
    		
    		//cout << "Process: " << world_rank << " | Chunk: " << iThreadChunk << " | Thread: " << iMyRank << " | Start Index: " << iThreadStartIndex << " | End Index: " << iThreadEndIndex << endl;

    		for(int i = iThreadStartIndex + 1; i < iThreadEndIndex - 1; i++) // Iteration through row
    		{
    			//if(i == 0 || i == iRowCount-1)
    				//continue;
    			
    			//cout << world_rank << "\t" << iMyRank << "\t" << i << endl;

    			//for(int i = 1; i < (iRowCount - 1); i++) || Wrong!!!!
					for(int j = 1; j < (iActualColumnCount - 1); j++) // Iteration through column
						iGridNew[i][j] = adjacentNeighbors(i, j);
    		}
    		//cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    	}
    	

		// Time to create a copy of the new generated state to the older one
		// as a reference to create the newer one in the next generation
		copyGrid(iRowCount, iActualColumnCount);
	}

	// Let all the processes get synchronized
	//MPI_Barrier(MPI_COMM_WORLD);

	/*cout << "Process " << world_rank << endl;

	for(int i = 1; i < (iRowCount - 1); i++)
	{
		for(int j = 1; j < (iActualColumnCount - 1); j++)
			cout << iGridNew[i][j] << " ";
		cout << endl;
	}*/

	// Copy the final copy to a 1D array for sending it to 
	if(world_rank)
	{
		//cout << "Process " << world_rank << " final copy." << endl;
		int iTempIndex = 0;
		int *iGridFinalLocal = new int[iRowCount*(iActualColumnCount - 2)];

		for(int i = 1; i < (iRowCount - 1); i++)
		{
			for(int j = 1; j < (iActualColumnCount - 1); j++)
			{
				iGridFinalLocal[iTempIndex] = iGrid[i][j];
				//cout << iGridFinalLocal[iTempIndex] << " ";
				iTempIndex += 1;
			}
		}

		// Sending the size of the row
		iRowCount = iRowCount - 2;
        MPI_Send(&iRowCount, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);

        //cout << "Process: " << world_rank << " | Row Size: " << iRowCount << " | Buffer Size: " <<  iTempIndex << endl;
        // Sending the buffer to process 0
        MPI_Send(iGridFinalLocal, iTempIndex, MPI_INT, 0, 1, MPI_COMM_WORLD);
	
        //cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

        // free the allocated array
		delete[] iGridFinalLocal;
		delete[] iGrid;
		delete[] iGridNew;
	}
	

	// Write the final state to the file
	else    
	{
		cout << "Job done, writing to the file." << endl;
		// First process 0 will write its own result to the file
		cout << "Process 0" << endl;
		for(int i = 1; i < (iRowCount - 1); i++)
		{
			for(int j = 1; j < (iActualColumnCount - 1); j++)
			{
				//cout << iGrid[i][j]  << " "; 
				fOutput << iGrid[i][j] << " ";
			}
			fOutput << endl;
			//cout << endl;
		}

		// Last, time  other processes' result
	    for(int i = 1; i < world_size; i++)
	    {
	    	// Receiving the size of row first
    		MPI_Recv(&iRowCount, 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	    	
    		int* iGridFinaLocalCopy = new int[iRowCount * (iActualColumnCount - 2)];

    		// Time to receive the final copy from other process
    		MPI_Recv(iGridFinaLocalCopy, (iRowCount * (iActualColumnCount - 2)), MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    		cout << "Process " << i << endl;

			// Writing to file
			for(int i = 0; i < (iRowCount * (iActualColumnCount - 2)); i++)
			{
				if(i % (iActualColumnCount - 2) == 0 && i != 0)
				{
					fOutput << endl;
					//cout << endl;
				}
				fOutput << iGridFinaLocalCopy[i] << " ";
				//cout << iGridFinaLocalCopy[i] << " ";
			}
			fOutput << endl;
			//cout << endl;

			// free the allocated array
			delete[] iGridFinaLocalCopy;
		}

		// Measure the ending clock time
        dEndTime = MPI_Wtime();
		
		// Calculate the time of the program
        cout << "Execution time: " << dEndTime - dStartTime << endl;

        cout << "\n\nProgram Configuration" << endl;
        cout << "Grid Size: " <<  iActualRowCount-2 << " * " << iActualColumnCount-2 << endl;
        cout << "Processes: " << world_se  << " | Threads: " << thread_count << endl;
        cout << "Number of generations: " << iGenerations << endl; 
		cout << "Last generation output has been written to file." << endl;
	}
	// Done reading from the file
	fInput.close();

	// Done writing to the file
	fOutput.close();

	// Finalize the MPI environment.
    MPI_Finalize();

	return 0;
}