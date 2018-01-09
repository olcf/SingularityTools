#include <stdio.h>
#include <string.h>
#include <mpi.h>

int main (int argc, char *argv[]) {
  int rank, size;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  FILE *lsb_proc = popen("lsb_release -ds", "r");
  char lsb_buf[1024];
  fgets(lsb_buf, 1024, lsb_proc);
  pclose(lsb_proc);
  strtok(lsb_buf, "\n");

  printf( "Hello from %s : rank  %d of %d\n", lsb_buf, rank, size);
 
  MPI_Finalize();

  return 0;
}

