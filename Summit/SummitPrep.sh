#!/bin/bash

# sudo singularity exec -w ./TitanPrep.sh

# Print commands executed
set -x

# Check to see if the singularity modulefile is loaded
echo '[ -z "$SINGULARITY_MODULE_LOADED" ] && echo "WARNING: singularity module not load!"' >> /environment

# Don't pass PYTHONSTARTUP into container
echo "unset PYTHONSTARTUP" >> /environment
# Don't pass PKG_CONFIG_PATH unless requested
echo '[ -z "$KEEP_PKG_CONFIG" ] && unset PKG_CONFIG_PATH' >> /environment

# Mount point for lustre
mkdir -p /lustre/atlas
mkdir -p /lustre/atlas1
mkdir -p /lustre/atlas2
