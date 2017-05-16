# Singularity on Titan
To ensure that `Singularity` containers are compatable with Titan several requirements must be met.

### CUDA
To enable the use of `NVCC` within the container version `7.5` of the `CUDA Toolkit` must be installed. Shared libraries and binaries(e.g. `libcuda.so` and `nvidia-smi`) which are installed as part of the `CUDA` driver, residing on the host,  will need to be exposed to the contianer. On Titan these files are located under `/opt/cray/nvidia/{VERSION}/lib64` and `/opt/cray/nvidia/{VERSION}/bin`. These directories are automatically mounted and visible within the container if the bind point `/opt/cray` exists in the container. Adding these directory path to `LD_LIBRARY_PATH` and `PATH` will allow these libraries and binaries to be resolved as need be.

### MPI
To run `MPI` applications within the container `MPI` must be built with Cray support. Although it is possible to build `OpenMPI` with Cray support it can be difficult and error prone within a container. An alternative way to provide `MPI` within a container is to create the container with the distro provided `MPICH` and before running the container on Titan overwrite the default `MPI` libraries with the Cray provided `MPI` libraries. Additionally the following bind mount directories must be made available with in the container: `/var/spool/alps` and `/var/opt/cray`.
