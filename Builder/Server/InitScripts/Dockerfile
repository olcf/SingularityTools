FROM ubuntu:xenial

RUN apt-get update
RUN apt-get install -y apt-utils build-essential wget python yum git autoconf libtool autogen squashfs-tools
RUN git clone -b development https://github.com/singularityware/singularity.git
RUN cd singularity && ./autogen.sh && ./configure --prefix=/usr/local && make && make install
RUN mkdir /work_dir

# This form is required so that singularity is not run directly and not in a shell so signals can be sent to it
CMD ["/usr/local/bin/singularity", "build", "./container.img", "./container.def"]
