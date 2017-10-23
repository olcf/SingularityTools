sudo apt-get update
sudo apt-get install -y apt-utils build-essential wget python yum git autoconf libtool autogen squashfs-tools
git clone -b development https://github.com/singularityware/singularity.git
cd singularity && ./autogen.sh && ./configure --prefix=/usr/local && make && sudo make install
