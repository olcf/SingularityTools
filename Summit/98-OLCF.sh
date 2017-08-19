if [ -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH="${OLCF_CONTAINER_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="${OLCF_CONTAINER_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
fi

if [ -z "$PATH" ]; then
    export PATH="${OLCF_CONTAINER_PATH}"
else
    export PATH="${OLCF_CONTAINER_PATH}:${PATH}"
fi
