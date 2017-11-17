#!/bin/bash

set -e

cd /home/cades

# Create not root builder user
sudo useradd --create-home --home-dir /home/builder --shell /bin/bash builder
# Allow builder user to run docker as sudo by adding to docker group
sudo gpasswd -a builder docker

# Create builder scratch work directory
mkdir /home/builder/container_scratch
sudo chown builder /home/builder/container_scratch
sudo chgrp builder /home/builder/container_scratch

# Create SSH key for builder
ssh-keygen -f /home/cades/BuilderKey
sudo chown cades /home/cades/BuilderKey

# Create singularity builder docker image
sudo docker build -t singularity_builder -f ${SCRIPTS_DIR}/Dockerfile .

# Update cmake version
cd /home/cades
sudo apt remove cmake
wget https://cmake.org/files/v3.9/cmake-3.9.5-Linux-x86_64.sh
sudo chmod +x ./cmake-3.9.5-Linux-x86_64.sh
yes | sudo ./cmake-3.9.5-Linux-x86_64.sh
sudo ln -s /home/cades/cmake-3.9.5-Linux-x86_64/bin/* /usr/local/bin

# Install a new version of boost
cd /home/cades
sudo apt-get install -y cmake
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
sudo ./b2 install

# Install ContainerBuilder
cd /home/cades
sudo apt-get install -y pkg-config
git clone https://github.com/AdamSimpson/ContainerBuilder.git
cd ContainerBuilder
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make
sudo make install

# Start the container builder service
cd /home/builder
su builder -c 'singularity instance.start ./ContainerBuilder builder'
