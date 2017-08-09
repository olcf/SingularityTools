# Singularity on Titan
To ensure that Singularity containers are compatable with Titan several factors must be considered.

### CUDA
To enable the use of `NVCC` within a container version 7.5 of the `CUDA Toolkit` must be installed. Shared libraries and binaries(e.g. `libcuda.so` and `nvidia-smi`) which are installed as part of the `NVIDIA` driver and version matched to the exact driver will need to be exposed to the contianer. On Titan these files are located under `/opt/cray/nvidia/{VERSION}/lib64` and `/opt/cray/nvidia/{VERSION}/bin`. These directories are automatically mounted and visible within the container if the bind point `/opt/cray` exists in the container. Titan ***does not** support the singularity flag `--nv`.

### MPI
To run `MPI` applications within the container `MPI` must be built with Cray support. Although it is possible to build `OpenMPI` with Cray support it can be difficult and error prone within a container. An alternative method to provide `MPI` within a container is to create the container with the distro provided `MPICH` and before running the container on Titan overwrite the default `MPICH` shared libraries with the Cray provided `MPICH` libraries. Additionally the following bind mount directories must be made available with in the container: `/var/spool/alps` and `/var/opt/cray`.

### dl-intercept
To facilitate making the `CUDA` driver libraries available at runtime and loading `Cray MPICH` libraries instead of the distro provided `MPICH` a small utility, dl-intercept](https://github.com/olcf/dl-intercept), has been created. This utility makes use of the [RTLD Audit interface](http://man7.org/linux/man-pages/man7/rtld-audit.7.html) to intercept and modify dynamic library loads. This library will be enabled inside of the container by loading the `singularity` module and should work regardless of how the container `LD_LIBRARY_PATH` is constructed or how the executed application was built(RPATH, RUNPATH, dlopen()).

### Scratch space
The following directories should be created within the container to enable access to `/lustre` scratch areas as well as directories required for CUDA and MPI support
```
# Mount point for Cray files
mkdir -p /opt/cray

# Mount point for Cray files needed for ALSP runtime
mkdir -p /var/spool/alps
mkdir -p /var/opt/cray

# Mount point for lustre
mkdir -p /lustre/atlas
mkdir -p /lustre/atlas1
mkdir -p /lustre/atlas2

# Mount point for /sw
mkdir -p /sw
mkdir -p /ccs/sw
mkdir -p /autofs/nccs-svm1_sw
```

