#!/bin/bash

apt-get source openmpi
cd openmpi-2.0.2

# Copy updated rules and control files which contain Cray specific configure options
cp ../DEB_files/rules debian
cp ../DEB_files/control debian
cp ../DEB_files/libopenmpi2.links.in debian

dpkg-source --commit

# This should probably be signed: https://help.ubuntu.com/community/GnuPrivacyGuardHowto
# The environment is preserved as we need cray pkg-config information
debuild --preserve-env -uc -us --lintian-opts --profile debian


