#!/bin/bash

cd /home/cades

# Fetch server init scripts
git clone https://github.com/olcf/SingularityTools.git

SCRIPTS_DIR=/home/cades/SingularityTools/Builder/Server/InitScripts

# Install vagrant
sudo apt-get install virtualbox
sudo apt-get install vagrant
# Install SQLite
sudo apt-get install -y sqlite3 libsqlite3-dev

# Create not root builder user
sudo useradd --create-home --home-dir /home/builder --shell /bin/bash builder

# Package builder vagrant box
cp ${SCRIPTS_DIR}/Vagrantfile /home/builder/Vagrantfile
sudo -u builder bash <<EOF
  vagrant up
  vagrant package --output singularity_builder.box
  vagrant box add ~/VagrantBase/singularity_builder.box --name SingularityBuilder

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

# Install a new version of boost
cd /home/cades
sudo apt-get install -y cmake
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
sudo ./b2 install

# Install SSH_Builder
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
