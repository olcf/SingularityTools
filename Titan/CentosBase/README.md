# Base container
The base container example provides a basis on which to build your own Singularity containers suitable for deployment on Titan. To facilitate building such containers a helper script, `TitanPrep.sh`, has been created which will automatically create the necessary bind points in the container. The container is still responsible for installing the `CUDA toolkit` and ensuring all `MPI` applications are built against `MPICH`. What follows is a basic `Centos 7` container build capable of supporting `MPI` and `CUDA` applications. It is assumed that you are building the container on a Linux system in which you have root privilege on or through a service such as `Singularity Hub`.

## Definition walkthrough
```sh
BootStrap: docker
From: centos:7

%post

# Install basic system software
yum -y install wget gcc-c++ redhat-lsb
yum -y install epel-release
yum repolist
```
These first few lines specify the base `Centos 7` container which will be pulled from `Docker Hub`; This is handy  as it removes any package manager dependencies on our host system although requires a few extra lines to setup a basic environment as shown.

```sh
# Install MPICH
yum -y install mpich mpich-devel
```
Installing the `Centos` provided `MPICH` package will serve as a base for building future packages within the container and will later be patched to be compatible with `Cray MPT`.

```sh
# Install the toolkit
wget http://developer.download.nvidia.com/compute/cuda/7.5/Prod/local_installers/cuda_7.5.18_linux.run
export PERL5LIB=.
sh cuda_7.5.18_linux.run --silent --toolkit --override
rm cuda_7.5.18_linux.run
```
The `CUDA Toolkit` can be installed using the `NVIDA` provided installation utility; The driver itself is excluded as it is maintained by the host OS. Care must be taken to ensure that the container makes available compilers which are compatible with `CUDA/7.5` and sets up appropriate environment variables.

```sh
# Install MPI4PY against mpich(python-mpi4py is built against OpenMPI)
yum -y install python-devel python-pip
pip install --upgrade pip

pip install mpi4py
```
With the base container setup we are now free to install packages utilizing MPI and CUDA. Care must be taken to ensure that all packages dependent on `MPI` are built against `MPICH`. For example the prebuilt `Centos` package `python-mpi4py` is built against `OpenMPI` and so must not be used; We can however install `mpi4py` with `pip` to ensure it will be built against the previously installed `MPICH` package.

```sh
# Patch container to work on Titan
wget https://raw.githubusercontent.com/olcf/SingularityTools/master/Titan/TitanPrep.sh
sh TitanPrep.sh
rm TitanPrep.sh
```
`TitanPrep.sh` is a small script to create necessary directories in the container used for bind mounting at runtime. Alternatively the `TitanPrep.sh` may be run, unprivileged, on the Titan internal login nodes after the container has been built.

## Building the container
```bash
$ sudo singularity create --size 8000 CentosTitan.img
$ sudo singularity bootstrap CentosTitan.img CentosTitan.def
```
Building the container does not require any Titan specific steps. The only care that must be taken is ensuring the container is large enough to handle the `CUDA Toolkit` installation. For our example application 8 gigabytes is sufficient.

```sh
%environment
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH-}:/usr/local/cuda/lib:/usr/local/cuda/lib64
export PATH=${PATH-}:/usr/local/cuda/bin:/usr/lib64/mpich/bin
```

Lastly ensure that any environment variables required at runtime are persisted.

## Transfering the container
Once the container has been built on a local resource it can be transferred to the OLCF using standard data transfer utilities. Currently Globus Online is the recommended way to facilitate this transfer.

## Running the container
```bash
$ module load singularity
$ singularity exec CentosTitan.img mpicc HelloMPI.c -o mpi.out
$ singularity exec CentosTitan.img nvcc HelloCuda.cu -o cuda.out
```
Within an interactive or batch job applications can be built and run utilizing the containers software stack. Full `/lustre` access is available from inside the container and the directory in which singularity is launched from will be the current working directory inside of the container. In this case the source code `HelloMPI.c`, `HelloCuda.cu`, and `HelloMPI.py` exists outside of the container on `lustre` and the applications `mpi.out` and `cuda.out` will be created in the same `lustre` directory. The singularity module sets environment variables which work in conjunction with the helper script `TitanPrep.sh` to ensure the `MPICH` and `CUDA` Titan specific patches work correctly.

```bash
$ aprun -n 2 -N 1 singularity exec CentosTitan.img ./mpi.out
Hello from Centos 7 : rank  0 of 2
Hello from Centos 7 : rank  1 of 2

$ aprun -n 1 singularity exec CentosTitan.img ./cuda.out
hello from the GPU

$ aprun -n 2 -N 1 singularity exec CentosTitan.img python HelloMPI.py 
Hello from mpi4py ('Centos', '7', 'zesty') : rank 1 of 2 
Hello from mpi4py ('Centos', '7', 'zesty') : rank 0 of 2
```
Once the applications have been built they can be executed on compute nodes through `aprun`. Note that the message `WARNING: Not mounting current directory: host does not support PR_SET_NO_NEW_PRIVS` can be ignored and should be removed in the next Singularity release.
