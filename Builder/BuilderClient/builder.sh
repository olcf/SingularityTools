#!/bin/bash

# To get the the nova command lines tool "sudo pip install python-novaclient"

if [ $# -ne 3 ]; then
  echo "Usage: builder name.img definition-file.def megabytes"
  exit 1
fi

IMG_PATH="$1"
IMG_BASENAME="$(basename $1)"
DEF_PATH="$2"
DEF_BASENAME="$(basename $2)"
CONTAINER_SIZE=$3

# Builder details
export VM_IP="1.2.3.4.5"
export KEY_FILE="ContainerBuilderKey"

# Socket file used for SSH control master on the host
CONTROL_SOCKET='/tmp/.controlmaster-%u-%r@%h:%p'

# Setup a control master socket so that all commands utilize the same connection
# This allows us to ensure that all subsuquent operations originate from the same user/session
/usr/bin/ssh -N -oControlMaster=yes -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE}, -oStrictHostKeyChecking=no builder@${VM_IP} &
CONTROL_MASTER_PID=$!

# Obtain the value of SSH_CONNECTION on the remote machine
WORK_PATH=$(/usr/bin/ssh -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE}, -oStrictHostKeyChecking=no builder@${VM_IP} 'GetWorkPath')

# On abnormal exit kill control master process
# Not all shells call EXIT on SIGHUP/INT/TERM
function cleanup {
  trap cleanup HUP INT TERM PIPE QUIT ABRT ERR
  if [ -n ${CONTROL_MASTER_PID} ]; then
    kill -9 ${CONTROL_MASTER_PID}
    unset CONTROL_MASTER_PID
  fi
}
trap cleanup HUP INT TERM PIPE QUIT ABRT ERR

# Copy definition file
# WORK_PATH will be created on behalf of this command
scp -S${CONTROL_SOCKET} -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no \
  ${DEF_PATH} "cades@${VM_IP}":${WORK_PATH}/container.def

# Build singularity container in docker
ssh -S${CONTROL_SOCKET} -t -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no "cades@${VM_IP}" \
  "/home/cades/DockerSingularityBuilder $CONTAINER_SIZE"

# Copy container file back
# WORK_PATH will be automatically deleted after this operation
scp -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no \
   "cades@${VM_IP}":"${WORK_PATH}/container.img" "${IMG_PATH}"

# Cleanup
unset WORK_PATH
kill -9 ${CONTROL_MASTER_PID}
unset CONTROL_MASTER_PID
