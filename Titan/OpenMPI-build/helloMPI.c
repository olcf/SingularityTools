#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "unistd.h"

int main (int argc, char *argv[], char** envp)
{
  int rank, size;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  char name[100];
  gethostname(name, 100);

  printf( "rank %d of %d on host %s with MPI_COMM_WORLD = %p \n", rank, size, name, MPI_COMM_WORLD);
/*
  if(rank == 0) {
    double buffer = 7.0;
    MPI_Bcast(&buffer, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  }
  else {
    char buffer;
    MPI_Bcast(&buffer, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
  }
*/
  MPI_Finalize();

  return 0;
}

