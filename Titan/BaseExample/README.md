# Build container
$ sudo singularity create --size 8000 ZestyTitan.img
$ sudo singularity bootstrap ZestyTitan.img Titan.def

# Test MPI
$ singularity exec ZestyTitan.img mpicc HelloMPI.c -o mpi.out
$ aprun -n 2 -N 1 singularity exec ZestyTitan.img ./mpi.out 
Hello from Ubuntu 17.04 : rank  0 of 2
Hello from Ubuntu 17.04 : rank  1 of 2

# Test CUDA
$ singularity exec ZestyTitan.img nvcc HelloCuda.cu -o cuda.out
$ aprun -n 1 singularity exec ZestyTitan.img ./cuda.out 
hello from the GPU
