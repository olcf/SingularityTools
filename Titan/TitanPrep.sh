#!/bin/bash

### Create directories and symlinks between /opt/cray/nvidia/lib64/* and /usr/lib
# sudo singularity exec -w ./TitanPrep.sh

# Print commands executed
set -x

# Don't pass PYTHONSTARTUP into container
echo "unset PYTHONSTARTUP" >> /environment
# Don't pass PKG_CONFIG_PATH unless requested
echo '[ -z "$KEEP_PKG_CONFIG" ] && unset PKG_CONFIG_PATH' >> /environment

####
# Setup Cray MPI
# These variables are expected to be set at container runtime(e.g. in the module file loading the container)
# SYSUTILS_DEFAULT_DIR=`readlink -f /opt/cray/sysutils/default`
# WLM_DEFAULT_DIR=`readlink -f /opt/cray/wlm_detect/default`
# CRAY_NVIDIA_DRIVER_LIB_DIR=`readlink -f /opt/cray/nvidia/default/lib64`
####

# Make sure Cray MPI/ALPS related libraries are in container LD_LIBRARY_PATH
echo 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${CRAY_LD_LIBRARY_PATH}:${SYSUTILS_DEFAULT_DIR}/lib64:${WLM_DEFAULT_DIR}/lib64' >> /environment

####
# Setup Cray-NVIDIA driver lib/bins
# These variables are expected to be set at container runtime(e.g. in the module file loading the container)
# CRAY_NVIDIA_DRIVER_LIB_DIR=`readlink -f /opt/cray/nvidia/default/lib64`
# CRAY_NVIDIA_DRIVER_BIN_DIR=`readlink -f /opt/cray/nvidia/default/bin`
####
echo 'export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${CRAY_NVIDIA_DRIVER_LIB_DIR}' >> /environment
echo 'export PATH=$PATH:${CRAY_NVIDIA_DRIVER_BIN_DIR}' >> /environment

# Mount point for Cray files
mkdir -p /opt/cray

# Mount point for Cray files needed for ALSP runtime
mkdir -p /var/spool/alps
mkdir -p /var/opt/cray

# Mount point for lustre
mkdir -p /lustre/atlas
mkdir -p /lustre/atlas1
mkdir -p /lustre/atlas2

### Create symlinks between libmpi* and Cray's mpich
c_mpich=`ldconfig -p | grep libmpich.so | awk 'NR==1 {print $4}' | xargs readlink -f`
cxx_mpich=`ldconfig -p | grep libmpichcxx.so | awk 'NR==1 {print $4}' | xargs readlink -f`
f_mpich=`ldconfig -p | grep libmpichfort.so | awk 'NR1==1 {print $4}' | xargs readlink -f`

if [ ! -f ${c_mpich} ]; then
  echo "libmpich.so not found!"
  exit 1
fi
if [ ! -f ${cxx_mpich} ]; then
  echo "libmpichcxx.so not found!"
  exit 1
fi
if [ ! -f ${f_mpich} ]; then
  echo "libmpichfort.so not found!"
  exit 1
fi

# Keep a copy of original
mv ${c_mpich} ${c_mpich}.original
mv ${cxx_mpich} ${cxx_mpich}.original
mv ${f_mpich} ${f_mpich}.original

# Replace MPICH libraries with symlink to Cray MPT libraries
ln -nsf /sw/titan/singularity/mpich/lib/libmpich.so ${c_mpich}
ln -nsf /sw/titan/singularity/mpich/lib/libmpichcxx.so ${cxx_mpich}
ln -nsf /sw/titan/singularity/mpich/lib/libfmpich.so ${f_mpich}
