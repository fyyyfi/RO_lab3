#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>

int ProcNum; // Number of the available processes
int ProcRank; // Rank of the current process
int *pParallelPivotPos; // Number of rows selected as the pivot ones
int *pProcPivotIter; // Number of iterations, at which the processor
// rows were used as the pivot ones
int *pProcInd; // Number of the first row located on the processes
int *pProcNum; // Number of the linear system rows located on the processes

// Function for simple definition of matrix and vector elements
void DummyDataInitialization (double* pMatrix, double* pVector, int Size) {
    int i, j; // Loop variables

    for (i=0; i<Size; i++) {
        pVector[i] = i+1;
        for (j=0; j<Size; j++) {
            if (j <= i)
                pMatrix[i*Size+j] = 1;
            else
                pMatrix[i*Size+j] = 0;
        }
    }
}

// Function for random definition of matrix and vector elements
void RandomDataInitialization (double* pMatrix,double* pVector,int Size) {
    int i, j; // Loop variables

    srand(unsigned(clock()));
    for (i=0; i<Size; i++) {
        pVector[i] = rand()/double(1000);
        for (j=0; j<Size; j++) {
            if (j <= i)
                pMatrix[i*Size+j] = rand()/double(1000);
            else
                pMatrix[i*Size+j] = 0;
        }
    }
}

// Function for memory allocation and data initialization
void ProcessInitialization (double* &pMatrix, double* &pVector,
double* &pResult, double* &pProcRows, double* &pProcVector,
double* &pProcResult, int &Size, int &RowNum) {
    int RestRows; // Number of rows, that haven't been distributed yet
    int i; // Loop variable

    if (ProcRank == 0) {
        do {
            printf("\nEnter the size of the matrix and the vector: ");
            scanf("%d", &Size);

            if (Size < ProcNum) {
                printf("Size must be greater than number of processes! \n");
            }
        }
        while (Size < ProcNum);
    }

    MPI_Bcast(&Size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    RestRows = Size;
    for (i=0; i<ProcRank; i++)
        RestRows = RestRows-RestRows/(ProcNum-i);
    RowNum = RestRows/(ProcNum-ProcRank);

    pProcRows = new double [RowNum*Size];
    pProcVector = new double [RowNum];
    pProcResult = new double [RowNum];

    pParallelPivotPos = new int [Size];
    pProcPivotIter = new int [RowNum];

    pProcInd = new int [ProcNum];
    pProcNum = new int [ProcNum];

    for (int i=0; i<RowNum; i++)
        pProcPivotIter[i] = -1;

    if (ProcRank == 0) {
        pMatrix = new double [Size*Size];
        pVector = new double [Size];
        pResult = new double [Size];

        DummyDataInitialization (pMatrix, pVector, Size);
        // RandomDataInitialization(pMatrix, pVector, Size);
    }
}
// Function for the data distribution among the processes
void DataDistribution(double* pMatrix, double* pProcRows, double* pVector,
double* pProcVector, int Size, int RowNum) {
    int *pSendNum; // Number of the elements sent to the process
    int *pSendInd; // Index of the first data element sent to the process
    int RestRows=Size; // Number of rows, that have not been distributed yet
    int i; // Loop variable

    // Alloc memory for temporary objects
    pSendInd = new int [ProcNum];
    pSendNum = new int [ProcNum];

    // Define the disposition of the matrix rows for the current process
    RowNum = (Size/ProcNum);
    pSendNum[0] = RowNum*Size;
    pSendInd[0] = 0;

    for (i=1; i<ProcNum; i++) {
        RestRows -= RowNum;
        RowNum = RestRows/(ProcNum-i);
        pSendNum[i] = RowNum*Size;
        pSendInd[i] = pSendInd[i-1]+pSendNum[i-1];
    }

    // Scatter the rows
    MPI_Scatterv(pMatrix, pSendNum, pSendInd, MPI_DOUBLE, pProcRows,
    pSendNum[ProcRank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Define the disposition of the matrix rows for current process
    RestRows = Size;
    pProcInd[0] = 0;
    pProcNum[0] = Size/ProcNum;

    for (i=1; i<ProcNum; i++) {
        RestRows -= pProcNum[i-1];
        pProcNum[i] = RestRows/(ProcNum-i);
        pProcInd[i] = pProcInd[i-1]+pProcNum[i-1];
    }

    MPI_Scatterv(pVector, pProcNum, pProcInd, MPI_DOUBLE, pProcVector,
    pProcNum[ProcRank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    // Free the memory
    delete [] pSendNum;
    delete [] pSendInd;
}

// Function for gathering the result vector
void ResultCollection(double* pProcResult, double* pResult) {
    // Gather the whole result vector on every processor
    MPI_Gatherv(pProcResult, pProcNum[ProcRank], MPI_DOUBLE, pResult,
    pProcNum, pProcInd, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

// Function for formatted matrix output
void PrintMatrix (double* pMatrix, int RowCount, int ColCount) {
    int i, j; // Loop variables

    for (i=0; i<RowCount; i++) {
        for (j=0; j<ColCount; j++)
            printf("%7.4f ", pMatrix[i*ColCount+j]);
        printf("\n");
    }
}

// Function for formatted vector output
void PrintVector (double* pVector, int Size) {
    int i;

    for (i=0; i<Size; i++)
        printf("%7.4f ", pVector[i]);
}

// Function for formatted vector output
void PrintResultVector (double* pResult, int Size) {
    int i;
    
    for (i=0; i<Size; i++)
        printf("%7.4f ", pResult[pParallelPivotPos[i]]);
}