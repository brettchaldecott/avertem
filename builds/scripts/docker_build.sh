#!/bin/bash


COMMAND=$1
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo ${WORK_DIR}

. ${WORK_DIR}/docker_env.sh

if [ -z "$COMMAND" ] || [ "$COMMAND" == "build" ]
then
    docker_start_build_container

    echo "Build Keto"
    docker_execute_command "/opt/keto/build.sh ubuntu build"

    docker_stop_build_container
elif [ "$COMMAND" == "clean" ]
then 
    echo "Clean docker"
    cd docker/compose-build && docker-compose down
fi
