#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Usage: builder name.img definition-file.def megabytes"
  exit 1
fi

# On exit kill control master process
# Not all shells call EXIT on SIGHUP/INT/TERM so we trap them
# Once we've trapped once ignore further abnormal traps(hitting ctrl-c a bunch)
function null_cleanup {
  trap null_cleanup HUP INT TERM PIPE QUIT ABRT
}
function cleanup {
  trap null_cleanup HUP INT TERM PIPE QUIT ABRT
  /usr/bin/ssh -O exit -S${CONTROL_SOCKET} -t -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP}
  exit
}
trap cleanup HUP INT TERM PIPE QUIT ABRT EXIT ERR

IMG_PATH="$1"
DEF_PATH="$2"

# Builder details
export VM_IP="128.219.187.226"
export KEY_FILE="./BuilderKey"

# Socket file used for SSH control master on the host
export CONTROL_SOCKET='~/.ssh/ControlSocket-%l%h%p%r'

# Setup a control master socket so that all commands utilize the same connection
# This allows us to ensure that all subsuquent operations originate from the same user/session
# https://en.wikibooks.org/wiki/OpenSSH/Cookbook/Multiplexing
/usr/bin/ssh -f -N -M -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP}

# Obtain the value of SSH_CONNECTION on the remote machine
WORK_PATH=$(/usr/bin/ssh -S${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP} 'GetWorkPath')

# Copy definition file
# WORK_PATH will be created on behalf of this command
/usr/bin/scp -oControlPath=${CONTROL_SOCKET} -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no ${DEF_PATH} builder@${VM_IP}:${WORK_PATH}/container.def

# Build singularity container in docker
/usr/bin/ssh -S${CONTROL_SOCKET} -t -F /dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP} /usr/local/bin/SingularityBuilder

# Copy container file back
# WORK_PATH will be automatically deleted after this operation
/usr/bin/scp -oControlPath=${CONTROL_SOCKET} -F/dev/null -i${KEY_FILE} -oStrictHostKeyChecking=no builder@${VM_IP}:${WORK_PATH}/container.name ${IMG_PATH}
