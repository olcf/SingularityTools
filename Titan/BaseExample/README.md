# Containers on Titan

## Titan compatability
To ensure that containers are compatable with Titan several requirements must be met.

### CUDA
To run `CUDA` applications version 7.5 of the `CUDA Toolkit` must be installed within the container. Shared libraries which are installed as part of the `CUDA` driver will need to be exposed to the contianer as well. The device driver libraries reside at `/opt/cray/nvidia/{VERSION}/lib64` which is automatically mounted and visible within the container assuming the bind point `/opt/cray` exists in the container. Adding this directory path to `LD_LIBRARY_PATH` will allow these driver libraries to be resolved as need be.

### MPI
To run `MPI` applications `MPI` within the container  must be built with Cray support. Although it is possible to build `OpenMPI` with Cray support it can be difficult and error prone within a container. An alternative way to provide `MPI` within a container is to create the container with the distro provided `MPICH` and before running the container on Titan overwrite the default `MPI` libraries with the Cray provided MPI libraries. Additionally several directories must be bind mounted within the container to facilitate `ALPS` communication.

## Example
To facilitate the above requirments a helper script, `TitanPrep.sh`, has been created which will automatically create the neccessary bind points, setup environment variables, and attempt to overwrite the `MPICH` shared libraries with symlinks which on Titan will resolve to the Cray provided `MPI` libraries. The container is still responsible for installing the `CUDA toolkit` and ensuring all `MPI` applications are built against `MPICH`  Example `Singularity` definition files are provided below.

```

```
