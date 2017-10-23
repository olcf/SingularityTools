#!/bin/bash

# Get script directory
SCRIPT_DIR=$(dirname $0)

# OpenStack credentials
source ${SCRIPT_DIR}/openrc.sh

# Remove Key
nova keypair-delete CadesKey
rm ${SCRIPT_DIR}/CadesKey
rm ${SCRIPT_DIR}/BuilderKey

# Remove IP file
rm ${SCRIPT_DIR}/BuilderIP

# Delete VM
nova delete ContainerBuilder
