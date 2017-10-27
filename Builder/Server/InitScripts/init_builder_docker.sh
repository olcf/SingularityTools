####
# Provision a VM capable
###

#!/bin/bash

set -e

cd /home/cades

# Fetch server init scripts
git clone https://github.com/olcf/SingularityTools.git

SCRIPTS_DIR=/home/cades/SingularityTools/Builder/Server/InitScripts

# Install docker
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce
# Install SQLite
sudo apt-get install -y sqlite3 libsqlite3-dev

# Add support of ppc64le arch
sudo apt-get install -y qemu binfmt-support zlib1g-dev libglib2.0-devlibpixman-1-dev libfdt-dev

# Install a newer qemu from source to support ppc64le
wget https://github.com/qemu/qemu/archive/v2.10.1.tar.gz
tar xf v2.10.1.tar.gz
cd qemu-2.10.1/
mkdir build
cd build
../configure --static
make
sudo make install

# Add ppc64le repos to apt sources
cat << EOF > /etc/apt/sources.list
deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ xenial main restricted multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ xenial-updates multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ xenial-backports main restricted universe multiverse
deb [arch=ppc64el] http://ports.ubuntu.com/ubuntu-ports/ xenial main restricted universe
deb [arch=ppc64el] http://ports.ubuntu.com/ubuntu-ports/ xenial-updates main restricted universe
EOF

# Install ppc64le glibc
sudo dpkg --add-architecture ppc64el
sudo apt-get update
sudo apt-get install libc6:ppc64el libc6-dev:ppc64el

# A dirty hack to copy the custom qemu static binaries over top of the distro provided dynamically linked binaries
sudo cp -r /usr/local/bin/qemu-* /usr/bin

# Docker by default seems to use aufs which isn't compatible with singularity(atleast when bootstrapping from docker)
# We force the use of overlayfs instead: https://docs.docker.com/engine/userguide/storagedriver/overlayfs-driver/#configure-docker-with-the-overlay-or-overlay2-storage-driver
sudo systemctl stop docker
cat << EOF > /etc/docker/daemon.json
{
  "storage-driver": "overlay2"
}
EOF
sudo systemctl start docker

# Enable apparmor profile
sudo apparmor_parser -r -W ${SCRIPTS_DIR}/apparmour.builder

# Create not root builder user
sudo useradd --create-home --home-dir /home/builder --shell /bin/bash builder
# Allow builder user to run docker as sudo by adding to docker group
sudo gpasswd -a builder docker

sudo -u builder bash <<EOF
  # Create the Builder database which contains two tables, a build_queue and active_builds
  # build_queue contains a list of queued builds
  # active_builds contains a single row and colum containing the number of active builds
  # We use AUTOINCREMENT so we can can sort our queue
  sqlite3 /home/builder/BuildQueue.db "CREATE TABLE queue(id INTEGER PRIMARY KEY, status TEXT, timestamp INTEGER DEFAULT (strftime('%s','now')));"
  sqlite3 /home/builder/ResourceManager.db "CREATE TABLE slot(id INTEGER PRIMARY KEY, status TEXT);"
  sqlite3 /home/builder/ResourceManager.db "INSERT INTO slot(status) VALUES('f');"
  sqlite3 /home/builder/ResourceManager.db "INSERT INTO slot(status) VALUES('f');"
  sqlite3 /home/builder/ResourceManager.db "INSERT INTO slot(status) VALUES('f');"
  sqlite3 /home/builder/ResourceManager.db "INSERT INTO slot(status) VALUES('f');"
EOF

# Create builder scratch work directory
mkdir /home/builder/container_scratch
sudo chown builder /home/builder/container_scratch
sudo chgrp builder /home/builder/container_scratch

# Create SSH key for builder
ssh-keygen -f /home/cades/BuilderKey
sudo chown cades /home/cades/BuilderKey

# Create singularity builder docker image
sudo docker build -t singularity_builder -f ${SCRIPTS_DIR}/Dockerfile .

# Create singularity ppc64le builder docker image
sudo docker build -t singularity_builder_ppc -f ${SCRIPTS_DIR}/Dockerfile.ppc .

# Install a new version of boost
cd /home/cades
sudo apt-get install -y cmake
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
sudo ./b2 install

# Install SSH_Builder
sudo apt-get install -y pkg-config
cd ${SCRIPTS_DIR}/../SafeSH
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" ..
make
sudo make install

# Add security options to lock down builder user SSH capabilities
SSH_KEY_OPTS='command=\"/usr/local/bin/SafeSH \\\"\${SSH_ORIGINAL_COMMAND}\\\"\",no-port-forwarding,no-X11-forwarding,no-agent-forwarding'
sudo sh -c "echo -n \"${SSH_KEY_OPTS} \" > /home/builder/.ssh/authorized_keys"

# Add newly created key to builders authorized_keys
sudo sh -c "cat /home/cades/BuilderKey.pub >> /home/builder/.ssh/authorized_keys"
