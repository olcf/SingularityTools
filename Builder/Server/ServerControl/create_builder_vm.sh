#!/bin/bash

# To get the the nova command lines tool "sudo pip install python-novaclient"
# @TODO call destroy_builder on ctrl-c

# Get script directory
SCRIPT_DIR=$(dirname $0)

# General VM settings
BOOTIMG="CADES_Ubuntu16.04_v20170804_1"
ZONE="nova"
FLAVOR="m1.medium"
NIC="or_provider_general_extnetwork1"

# OpenStack credentials
source ./openrc.sh

# Create Keys for cades user
KEY="CadesKey"
KEY_FILE="${SCRIPT_DIR}/${KEY}"

nova keypair-add ${KEY} > ${KEY_FILE}
chmod 600 ${KEY_FILE}

echo "This make take some time."

# Startup new VM
VM_UUID=$(nova boot                                          \
    --image "${BOOTIMG}"                                     \
    --flavor "${FLAVOR}"                                     \
    --availability-zone "${ZONE}"                            \
    --nic net-name="${NIC}"                                  \
    --key-name "${KEY}"                                      \
    --user-data ${SCRIPT_DIR}/../ServerScripts/init_builder.sh  \
    "ContainerBuilder" | awk '/id/ {print $4}' | head -n 1);

# Spinner...
function spin_me_right_round() {
  declare -a spin=("-" "\\" "|" "/"
                   "-" "\\" "|" "/"
                   "-" "\\" "|" "/")
  for i in "${spin[@]}"
  do
        echo -ne "\b$i"
        sleep 0.5
  done
}

# Wait for VM to become active.
until [[ "$(nova show ${VM_UUID} | awk '/status/ {print $4}')" == "ACTIVE" ]]; do
  spin_me_right_round
done
# Wait for SSH to be usable.
until nova console-log ${VM_UUID} | grep "running 'modules:final'" > /dev/null 2>&1; do
  spin_me_right_round
done

# Get external IP address
VM_IP=`nova show ${VM_UUID} | grep or_provider_general_extnetwork1 | awk '{print $5}'`

rm -rf ContainerBuilderIP
echo $VM_IP > ${SCRIPT_DIR}/ContainerBuilderIP

# Retrieve private key for builder
scp -i $KEY_FILE cades@${VM_IP}:/home/cades/BuilderKey ${SCRIPT_DIR}

echo "Started ${VM_UUID} with external IP ${VM_IP} using ${KEY_FILE}"
