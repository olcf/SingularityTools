#!/bin/bash

# To get the the nova command lines tool "sudo pip install python-novaclient"

if [ $# -ne 3 ]; then
  echo "Usage: builder name.img definition-file.def megabytes"
  exit 1
fi

# On abnormal exit kill control master process
# Not all shells call EXIT on SIGHUP/INT/TERM
function cleanup {
  trap cleanup HUP INT TERM PIPE QUIT ABRT ERR EXIT
  if [ -n "${CONTROL_MASTER_PID}" ]; then
    kill -9 ${CONTROL_MASTER_PID}
    unset CONTROL_MASTER_PID
  fi
  if [ -n "${CONTROL_SOCKET_DIR}" ]; then
    rm -r ${CONTROL_SOCKET_DIR}
    unset CONTROL_SOCKET_DIR
  fi
}
trap cleanup HUP INT TERM PIPE QUIT ABRT ERR EXIT

IMG_PATH="$1"
IMG_BASENAME="$(basename $1)"
DEF_PATH="$2"
DEF_BASENAME="$(basename $2)"
CONTAINER_SIZE=$3

# Builder details
export VM_IP="128.219.187.223"
export KEY_FILE="./BuilderKey"

# Socket file used for SSH control master on the host
export CONTROL_SOCKET_DIR=$(mktemp -d "/tmp/control_master.XXXXXX")
export CONTROL_SOCKET="${CONTROL_SOCKET_DIR}/control_socket"

# Setup a control master socket so that all commands utilize the same connection
# This allows us to ensure that all subsuquent operations originate from the same user/session
/usr/bin/ssh -N -oControlMaster=yes -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP} &
CONTROL_MASTER_PID=$!

# Obtain the value of SSH_CONNECTION on the remote machine
WORK_PATH=$(/usr/bin/ssh -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP} 'GetWorkPath')

# Copy definition file
# WORK_PATH will be created on behalf of this command
/usr/bin/scp -o ControlPath=${CONTROL_SOCKET} -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no ${DEF_PATH} builder@${VM_IP}:${WORK_PATH}/container.def

# Build singularity container in docker
ssh -S${CONTROL_SOCKET} -t -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP} /usr/local/bin/SingularityBuilder $CONTAINER_SIZE

# Copy container file back
# WORK_PATH will be automatically deleted after this operation
/usr/bin/scp -o ControlPath=${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP}:${WORK_PATH}/container.img ${IMG_PATH}
