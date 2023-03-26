#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

//Read numbers from file as vector of ints
bool getNumbersFromFile(vector<int> *numbers) {
    int number;                                    
    fstream myFile;
    myFile.open("numbers", ios::in); 
        
    if (!myFile.is_open())
    {
        cerr << "File could not be opened" << endl;
        return false;
    }
    
    while(myFile.good()){
        number = myFile.get();
        
        if(!myFile.good())
            break;
        
        numbers->push_back(number);
    }
    myFile.close();    
    return true; 
}

int main(int argc, char* argv[]) {
    //Initializations--------------------------------------------------------------------------------
    int size, rank;
    int midian = 0;
    vector<int> numbers;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //check if there is enough processes
    if (size < 2) {
        cerr << "Not enough processes" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    //Getting numbers and scattering--------------------------------------------------------------------------------

    //rank 0 reads numbers from the file and calculates midian and size of numbers
    int* numbersArray = nullptr;
    int numbersSize = 0;
    if (rank == 0) {
        getNumbersFromFile(&numbers);
        numbersSize = numbers.size();

        //check if size of numbers is divisible by number of processes
        if (numbersSize % size != 0) {
            cerr << "Size of numbers can not be divisible by number of processes" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        //copy vector to array
        numbersArray = new int[numbersSize];
        for (int i = 0; i < numbersSize; i++) {
            numbersArray[i] = numbers[i];
        }
        //broadcast size of numbers to all processes
        MPI_Bcast(&numbersSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

        //find midian
        if (numbersSize % 2 == 0) {
            midian = numbers[numbersSize / 2 - 1];
        } else {
            midian = numbers[numbersSize / 2];
        }
        //broadcast midian to all processes
        MPI_Bcast(&midian, 1, MPI_INT, 0, MPI_COMM_WORLD);

    }
    //receive size of numbers from process 0
    MPI_Bcast(&numbersSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //receive midian from process 0
    MPI_Bcast(&midian, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate space for a portion of the array on each process
    int localRecieveCount = numbersSize / size;
    int* localNumbersArray = new int[localRecieveCount];
    // Scatter the array to all processes
    MPI_Scatter(numbersArray, localRecieveCount, MPI_INT, localNumbersArray, localRecieveCount, MPI_INT, 0, MPI_COMM_WORLD);

    //LESS--------------------------------------------------------------------------------

    //get all numbers smaller than midian and add them to local array
    int* localLesser = new int[localRecieveCount];
    int localLesserCount = 0;
    for (int i = 0; i < localRecieveCount; i++) {
        if (localNumbersArray[i] < midian) {
            localLesser[localLesserCount] = localNumbersArray[i];
            localLesserCount++;
        }
    }

    //Gathar size of numbers smaller than midian to process 0
    int *recvCountsLesser = NULL;
    if (rank == 0){
      recvCountsLesser =(int *) malloc( size * sizeof(int)) ;
    }
    MPI_Gather(&localLesserCount, 1, MPI_INT,
        recvCountsLesser, 1, MPI_INT,
        0, MPI_COMM_WORLD);
    //Gathar all numbers smaller than midian to process 0
    int totalLenLesser = 0;
    int *displs = NULL;
    int *allLesser = NULL;
    if (rank == 0) {
        displs = (int *) malloc( size * sizeof(int)) ;
        displs[0] = 0;
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i - 1] + recvCountsLesser[i - 1];
        }
        totalLenLesser = displs[size - 1] + recvCountsLesser[size - 1];
        allLesser = (int *) malloc( totalLenLesser * sizeof(int)) ;
    }
    MPI_Gatherv(localLesser, localLesserCount, MPI_INT,
        allLesser, recvCountsLesser, displs, MPI_INT,
        0, MPI_COMM_WORLD);


    //GREAT--------------------------------------------------------------------------------
    //get all numbers greater than midian and add them to local array
    int* localGreater = new int[localRecieveCount];
    int localGreaterCount = 0;
    for (int i = 0; i < localRecieveCount; i++) {
        if (localNumbersArray[i] > midian) {
            localGreater[localGreaterCount] = localNumbersArray[i];
            localGreaterCount++;
        }
    }

    //Gathar size of numbers greater than midian to process 0
    int *recvCountsGreater = NULL;
    if (rank == 0){
      recvCountsGreater =(int *) malloc( size * sizeof(int)) ;
    }
    MPI_Gather(&localGreaterCount, 1, MPI_INT,
        recvCountsGreater, 1, MPI_INT,
        0, MPI_COMM_WORLD);
    //Gathar all numbers greater than midian to process 0
    int totalLenGreater = 0;
    int *displsGreater = NULL;
    int *allGreater = NULL;
    if (rank == 0) {
        displsGreater = (int *) malloc( size * sizeof(int)) ;
        displsGreater[0] = 0;
        for (int i = 1; i < size; i++) {
            displsGreater[i] = displsGreater[i - 1] + recvCountsGreater[i - 1];
        }
        totalLenGreater = displsGreater[size - 1] + recvCountsGreater[size - 1];
        allGreater = (int *) malloc( totalLenGreater * sizeof(int)) ;
    }
    MPI_Gatherv(localGreater, localGreaterCount, MPI_INT,
        allGreater, recvCountsGreater, displsGreater, MPI_INT,
        0, MPI_COMM_WORLD);


    //EQUAL--------------------------------------------------------------------------------
    //gather all equal numbers than midian to process 0
    int* localEqual = new int[localRecieveCount];
    int localEqualCount = 0;
    for (int i = 0; i < localRecieveCount; i++) {
        if (localNumbersArray[i] == midian) {
            localEqual[localEqualCount] = localNumbersArray[i];
            localEqualCount++;
        }
    }

    //Gathar size of numbers equal to midian to process 0
    int *recvCountsEqual = NULL;
    if (rank == 0){
      recvCountsEqual =(int *) malloc( size * sizeof(int)) ;
    }
    MPI_Gather(&localEqualCount, 1, MPI_INT,
        recvCountsEqual, 1, MPI_INT,
        0, MPI_COMM_WORLD);
    //Gathar all numbers equal to midian to process 0
    int totalLenEqual = 0;
    int *displsEqual = NULL;
    int *allEqual = NULL;
    if (rank == 0) {
        displsEqual = (int *) malloc( size * sizeof(int)) ;
        displsEqual[0] = 0;
        for (int i = 1; i < size; i++) {
            displsEqual[i] = displsEqual[i - 1] + recvCountsEqual[i - 1];
        }
        totalLenEqual = displsEqual[size - 1] + recvCountsEqual[size - 1];
        allEqual = (int *) malloc( totalLenEqual * sizeof(int)) ;
    }
    MPI_Gatherv(localEqual, localEqualCount, MPI_INT,
        allEqual, recvCountsEqual, displsEqual, MPI_INT,
        0, MPI_COMM_WORLD);


    //print all Lesser than midian
    if (rank == 0) {
        cout << "L: ";
        for (int i = 0; i < totalLenLesser; i++) {
            cout << allLesser[i] << " ";
        }
        cout << endl;
    }

    //print all Equal to midian
    if (rank == 0) {
        cout << "E: ";
        for (int i = 0; i < totalLenEqual; i++) {
            cout << allEqual[i] << " ";
        }
        cout << endl;
    }

    //print all Greater than midian
    if (rank == 0) {
        cout << "G: ";
        for (int i = 0; i < totalLenGreater; i++) {
            cout << allGreater[i] << " ";
        }
        cout << endl;
    }

    // Clean up memory
    delete[] numbersArray;
    delete[] localNumbersArray;
    delete[] localLesser;
    delete[] localGreater;
    delete[] localEqual;
    // End
    MPI_Finalize();
    return 0;
}