#!/bin/bash

####
## Append to #post in bootstrap or run after container creation
## sudo singularity exec -w ./TitanPrep.sh
####

# Print commands executed
set -x

# Mount point for Cray files
mkdir -p /opt/cray
mkdir -p /opt/nvidia

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

# Mount point for proj read-only dirs
mkdir -p /ccs/proj
mkdir -p /autofs/nccs-svm1_proj
