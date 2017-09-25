#!/bin/bash

# Get script directory
SCRIPT_DIR=$(dirname $0)

# OpenStack credentials
source ${SCRIPT_DIR}/openrc.sh

# Remove Key
nova keypair-delete ContainerBuilderKey
rm ${SCRIPT_DIR}/ContainerBuilderKey

# Remove IP file
rm ${SCRIPT_DIR}/ContainerBuilderIP

# Delete VM
nova delete ContainerBuilder
