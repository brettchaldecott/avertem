#!/bin/bash


COMMAND=$1
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo ${WORK_DIR}

. ${WORK_DIR}/docker_env.sh

if [ -z "$COMMAND" ] || [ "$COMMAND" == "ide" ]
then
    IDE=$2
    if [ -z "$IDE" ]
    then
        echo "Must provide the IDE"
        exit -1
    fi
    COPY_DEPENDENCIES=$3
    docker_start_build_container

    echo "Generate Avertem IDE - $IDE"
    docker_execute_command "/opt/avertem/builds/scripts/generator_in_docker.sh ubuntu \"$IDE\" \"$COPY_DEPENDENCIES\""
fi
