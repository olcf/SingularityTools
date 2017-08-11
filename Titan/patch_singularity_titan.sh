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
                        "autofs/nccs-svm1_sw")

mkdir -p .singularity-fixer && cd .singularity-fixer

for dir in "${directories[@]}"; do
  mkdir -p $dir
  tar -cp $dir | singularity import $image
done

cd ..
rm -rf .singularity-fixer
