#!/bin/bash


COMMAND=$1

if [ -z "${COMMAND}" ] ;
then
    echo "docker_build: must provide a command"
    echo "  build - build using docker"
    echo "  genesis - contract genesis"
    echo "  stop - stop the docker environment"
    echo "  clean - stop the docker environment"
    exit 1
fi

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo ${WORK_DIR}

. ${WORK_DIR}/docker_env.sh

if [ "$COMMAND" == "build" ]
then
    docker_start_build_container

    echo "Build Avertem"
    docker_execute_command "/opt/avertem/build.sh ubuntu build"

elif [ "$COMMAND" == "genesis" ]
then
    docker_start_build_container

    echo "Build Genesis"
    docker_execute_command "/opt/avertem/builds/scripts/update_genisis.sh"

elif [ "$COMMAND" == "start" ]
then 
    docker_start_build_container
elif [ "$COMMAND" == "stop" ]
then 
    docker_stop_build_container_env
elif [ "$COMMAND" == "clean" ]
then 
    docker_clean_build_container
fi
