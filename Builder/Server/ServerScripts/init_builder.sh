#!/bin/bash

cd /home/cades

# Fetch server init scripts
git clone https://github.com/olcf/SingularityTools.git

SCRIPTS_DIR=/home/cades/SingularityTools/Builder/Server/ServerScripts

# Install docker
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce

# Enable apparmor profile
sudo apparmor_parser -r -W ${SCRIPTS_DIR}/apparmour.builder

# Create builder user whichout shell access
echo "/usr/sbin/nologin" >> /etc/shells
sudo useradd --create-home --home-dir /home/builder --shell /usr/sbin/nologin builder

# Allow builder user to run docker as sudo by adding to docker group
sudo gpasswd -a builder docker

# Increase the number of loop devices
LOOP_MAX=63
for i in $(seq 8 ${LOOP_MAX}); do
  sudo mknod -m 660 "/dev/loop${i}" b 7 "$i"
done

# Create file containing available loop devices
LOOP_FILE="/home/builder/AvailableLoopDevices"
touch $LOOP_FILE
for i in $(seq 0 ${LOOP_MAX}); do
  echo $i >> $LOOP_FILE
done

# Create file for simple user queue
touch /home/builder/BuildQueue

# Create singularity builder docker image
sudo docker build -t singularity_builder -f ${SCRIPTS_DIR}/Dockerfile  .

# Create builder scratch work directory
mkdir /home/builder/container_scratch

# Create SSH key for builder
ssh-keygen -f /home/cades/BuilderKey

# Build the the Command Sanitizer application, providing it the builder IP
# This requires a recent version of boost and so we
sudo apt-get install -y cmake
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
tar xf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
sudo ./b2 install

BUILDER_IP=$(ifconfig ens3 | awk '/inet addr/ {gsub("addr:", "", $2); print $2}')
cd ${SCRIPTS_DIR}/../SSH_Sanitizer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX="/usr/local" -DBUILDER_IP="\"${BUILDER_IP}\"" ..
make
make install

# Install SingularityBuilder
cp ${SCRIPTS_DIR}/SingularityBuilder /usr/local/bin

# Add security options to lock down builder user SSH capabilities
SSH_KEY_OPTS='command="/usr/local/SSH_Sanitizer",no-port-forwarding,no-X11-forwarding,no-agent-forwarding,no-pty '
sudo sh -c "echo -n ${SSH_KEY_OPTS} > /home/builder/.ssh/authorized_keys"

# Add newly created key to builders authorized_keys
sudo sh -c "cat /home/cades/BuilderKey.pub >> /home/builder/.ssh/authorized_keys"
