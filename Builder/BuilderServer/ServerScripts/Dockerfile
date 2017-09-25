FROM ubuntu:xenial
  
RUN apt-get update
RUN apt-get install -y apt-utils build-essential wget python yum git autoconf libtool autogen
RUN git clone -b development https://github.com/singularityware/singularity.git
RUN cd singularity && ./autogen.sh && ./configure --prefix=/usr/local && make && make install
RUN mkdir /work_dir
