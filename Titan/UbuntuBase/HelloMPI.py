from mpi4py import MPI
import sys
import platform

size = MPI.COMM_WORLD.Get_size()
rank = MPI.COMM_WORLD.Get_rank()

sys.stdout.write("Hello from mpi4py %s : rank %d of %d \n" % (platform.linux_distribution(), rank, size))
