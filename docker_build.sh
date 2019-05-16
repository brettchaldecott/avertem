#!/bin/bash

COMMAND=$1
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "$COMMAND" ] || [ "$COMMAND" == "build" ]
then
    echo "Start docker"
    cd docker/compose-build && docker-compose up -d
    cd ${WORK_DIR}


    echo "Build Keto"
    docker exec -it compose-build_build-container_1 bash -c "/opt/keto/build.sh ubuntu build"

    echo "Stop docker"
    cd docker/compose-build && docker-compose stop
elif [ "$COMMAND" == "clean" ]
then 
    echo "Clean docker"
    cd docker/compose-build && docker-compose down
fi
