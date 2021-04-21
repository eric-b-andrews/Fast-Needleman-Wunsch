#include <chrono>
#include <iostream>

// TODO
// Try horizontal strips over vertical
//  (tune omp to hande that better)
//  can use memcpy for communication
//  might not even need a dedicated buffer--can use the table itself
//  this would allow us to use Isend...

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("error: incorrect number of arguments (expected 2, got %i)\n",
           argc - 1);
    return 1;
  }
      
  MPI_Init(NULL, NULL);
  int nProc, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  dnaArray s1, s2;
  try {
    s1 = readSequence(argv[1]);
    s2 = readSequence(argv[2]);
  }

  catch (std::string e) {
    std::cout << "ERROR: no such file " << e << std::endl;
    printf("ERROR: file not found\n");
    return 1;
  }

  long int nCols = ((s1.size + 1) / nProc) + (rank > 0);
  if (rank == nProc - 1) { nCols += (s1.size + 1) % nProc; }
  long int size = nCols * (long int)(s2.size + 1);
  int* table = new int[size];
  for (long int i = 0; i < size; i += 1024) { table[i] = 0; }

  // get start time in ms since epoch
  long int wallStart = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  // print start time
  // printf("P%i | wallStart: %li\n", rank, wallStart);

  // run the algorithm
  needlemanWunsch(s1, s2, nCols, rank, nProc, table);

  // get end time in ms since epoch
  long int wallEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  // print end time
  // printf("P%i | wallEnd: %li\n", rank, wallEnd);

  MPI_Barrier(MPI_COMM_WORLD);


  // collect and display time info on rank 0
  if (rank == 0) {
    // first process to start--used as start for timer
    long int first = wallStart;

    // collect start times, get minimum
    long int recStart, recEnd;
    for (int i = 1; i < nProc; ++i) {
      MPI_Recv(&recStart, 1, MPI_LONG, i, i, MPI_COMM_WORLD, NULL);
      first = (recStart < first) ? recStart: first;
    }

    // get last time (guaranteed to be last process by data deps)
    MPI_Recv(&recEnd, nProc-1, MPI_LONG, nProc-1, nProc-1,
             MPI_COMM_WORLD, NULL);
    
    printf("time: %li\n", recEnd - first);
  }

  // send time info to rank 0
  else {
    MPI_Send(&wallStart, 1, MPI_LONG, 0, rank, MPI_COMM_WORLD);
  }
  if (rank == nProc - 1) {
    MPI_Send(&wallEnd, 1, MPI_LONG, 0, rank, MPI_COMM_WORLD);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // display final score
  if (rank == nProc - 1) {
    printf("final score: %i\n", table[size-1]);
  }
  
  MPI_Finalize();
  
  return 0;
}
