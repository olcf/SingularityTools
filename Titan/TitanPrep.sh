#!/bin/bash

image=../$1
declare -a directories=("opt/cray"
                        "var/spool/alps"
                        "var/opt/cray"
                        "lustre/atlas"
                        "lustre/atlas1"
                        "lustre/atlas2"
                        "sw"
                        "ccs/sw"
                        "autofs/nccs-svm1_sw"
                        "ccs/proj"
                        "autofs/nccs-svm1_proj")

mkdir -p .singularity-fixer && cd .singularity-fixer

for dir in "${directories[@]}"; do
  mkdir -p $dir
  tar -cp $dir | singularity import $image
done

mkdir -p .singularity/env
touch 98-OLCF.sh
tar -cp .singularity/env/98-OLCF.sh | singularity import $image

cd .. && rm -rf .singularity-fixer
