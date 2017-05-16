# Example: Base container
To facilitate building containers suitable for deployment on Titan a helper script, `TitanPrep.sh`, has been created which will automatically create the neccessary bind points, setup environment variables, and attempt to overwrite the distro installed `MPICH` shared libraries with symlinks which on Titan will resolve to the Cray provided `MPI` libraries. The container is still responsible for installing the `CUDA toolkit` and ensuring all `MPI` applications are built against `MPICH`. What follows is a basic `Ubuntu Zesty(17.4)` container build capable of building `MPI` and `CUDA` applications. It is assumed that you are building the container on a Linux system you have root privilidge on or through a service such as `Singularity Hub`.

## Definition walkthrough
```
BootStrap: docker
From: ubuntu:zesty

%post
# Set PATH and LD_LIBRARY_PATH
export PATH=/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin
export LD_LIBRARY_PATH=/usr/local/lib:/lib64/usr/lib/x86_64-linux-gnu

# Install MPICH
apt-get install -y software-properties-common wget pkg-config
apt-add-repository universe
apt-get update
```
These first few lines specify the base `Ubuntu Zesty` container which will be pulled from `Docker Hub`; This is handy  as it removes any package manager dependencies on our host system although requires a few extra lines to setup a basic environment as shown.

```
apt-get install -y mpich
```
Installing the distro provided `MPICH` package will serve as a base for building future packages within the container and will later be patched to be comptabile with `Cray MPT`.

```
# Install the toolkit
wget http://developer.download.nvidia.com/compute/cuda/7.5/Prod/local_installers/cuda_7.5.18_linux.run
export PERL5LIB=.
sh cuda_7.5.18_linux.run --silent --toolkit --override
rm cuda_7.5.18_linux.run

# Patch CUDA toolkit to work with non system default GCC
ln -s /usr/bin/gcc-4.9 /usr/local/cuda/bin/gcc
ln -s /usr/bin/g++-4.9 /usr/local/cuda/bin/g++

# Set CUDA env variables
export PATH=$PATH:/usr/local/cuda/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib:/usr/local/cuda/lib64
```
The `CUDA Toolkit` can be installed using the `NVIDA` provided installation utility; The driver itself is excluded as it will be maintained by the host OS. Care must be taken to ensure that the container makes available compilers which are compatable with `CUDA/7.5` and sets up appropriate environment variables.

```
# Persist PATH and LD_LIBRARY_PATH to container runtime
echo "" >> /environment
echo "export PATH=${PATH}" >> /environment
echo "export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >> /environment
```
`Singularity` sources the file `/environment` during container runtime, here we ensure that any environment variables required at runtime are persisted.

```
# Patch container to work on Titan
wget https://raw.githubusercontent.com/olcf/SingularityTools/master/Titan/TitanPrep.sh
sh TitanPrep.sh
rm TitanPrep.sh
```
Lastly `TitanPrep.sh` is run, setting up appropriate bindpoints and patching the `Ubuntu` provided `MPICH` installation. At this point the container will not function properly on any system but Titan.

## Building the container
```
$ sudo singularity create --size 8000 ZestyTitan.img
$ sudo singularity bootstrap ZestyTitan.img Titan.def
```
Building the container is straight forward, the only care that must be taken is ensuring the container is large enough to handle the `CUDA Toolkit` installation. For our example application 8 gigabytes is sufficient.

## Transfering the container
Once the container has been built on a local resource it can be transfered to the OLCF. Currently Globus Online is the recomended way to facilitate this transfer.

## Running the container
```
$ singularity exec ZestyTitan.img mpicc HelloMPI.c -o mpi.out
$ singularity exec ZestyTitan.img nvcc HelloCuda.cu -o cuda.out
```
From within an interactive or batch job applications can be built utilizing the containers software stack. 

```
$ aprun -n 2 -N 1 singularity exec ZestyTitan.img ./mpi.out 
Hello from Ubuntu 17.04 : rank  0 of 2
Hello from Ubuntu 17.04 : rank  1 of 2

$ aprun -n 1 singularity exec ZestyTitan.img ./cuda.out 
hello from the GPU
```
Once the applications have been built they can be executed on compute nodes through `aprun`.
