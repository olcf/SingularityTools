# Singularity on Titan
To ensure that Singularity containers are compatible with Titan several factors must be considered.

### CUDA
To enable the use of `NVCC` within a container version 7.5 of the `CUDA Toolkit` must be installed. Shared libraries and binaries(e.g. `libcuda.so` and `nvidia-smi`) which are installed as part of the `NVIDIA` driver and version matched to the exact driver will need to be exposed to the container. On Titan these files are located under `/opt/cray/nvidia/{VERSION}/lib64` and `/opt/cray/nvidia/{VERSION}/bin`. These directories are automatically mounted and visible within the container if the bind point `/opt/cray` exists in the container. Titan **does not** support the singularity flag `--nv`.

### MPI
To run `MPI` applications within the container `MPI` must be built with Cray support. The recommended method to support  `MPI` within a container is to create the container with the distro provided `MPICH` and before running the container on Titan "swap" the default `MPICH` shared libraries with the Cray provided `MPICH` libraries. Additionally the following bind mount directories must be made available with in the container: `/var/spool/alps` and `/var/opt/cray`. Although it is possible to build `OpenMPI` with Cray support it can be difficult and error prone within a container and is therefore not recommended.

### dl-intercept
To facilitate swaping the distro provided `MPICH` libraries for the `Cray MPICH` libraries a small utility, [dl-intercept](https://github.com/olcf/dl-intercept), has been created. This utility makes use of the [RTLD Audit interface](http://man7.org/linux/man-pages/man7/rtld-audit.7.html) to intercept and modify dynamic library loads. This library will be enabled inside of the container by loading the `singularity` module and should work regardless of how the container `LD_LIBRARY_PATH` is constructed or how the executed application was built(RPATH, RUNPATH, dlopen()).

### Bind point
The following directories should be created within the container to enable access to `/lustre` scratch areas as well as directories required for CUDA and MPI support
```
# Mount point for Cray files
/opt/cray

# Mount point for Cray files needed for ALSP runtime
/var/spool/alps
/var/opt/cray

# Mount point for lustre
/lustre/atlas
/lustre/atlas1
/lustre/atlas2

# Mount point for /sw
/sw
/ccs/sw
/autofs/nccs-svm1_sw

# Mount point for proj read-only dirs
mkdir -p /ccs/proj
mkdir -p /autofs/nccs-svm1_proj

# Mount point stub file for init script
/.singularity.d/env/98-OLCF.sh
```
To facilitate this it is recomended that the script `TitanBootstrap.sh` be run in the `%post` section of your container definition. It is also possible to attempt to do this after container creation using the `TitanPrep.sh` script.
