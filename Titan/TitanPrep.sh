#!/bin/bash

####
## Append to #post in bootstrap or run after container creation
## sudo singularity exec -w ./TitanPrep.sh
####

# Print commands executed
set -x

# Mount point for Cray files
mkdir -p /opt/cray

# Mount point for Cray files needed for ALSP runtime
mkdir -p /var/spool/alps
mkdir -p /var/opt/cray

# Mount point for lustre
mkdir -p /lustre/atlas
mkdir -p /lustre/atlas1
mkdir -p /lustre/atlas2
