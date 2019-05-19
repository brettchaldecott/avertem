#!/bin/bash

lib_dir() {
    echo "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
}

docker_execute_command() {
    COMMAND=$1
    if [ -z "$COMMAND" ] ;
    then
        echo "Docker command not provided"
        exit -1
    fi
    docker exec -it compose-build_build-container_1 bash -c "$COMMAND"
}


docker_start_build_container() {
    echo "Start docker"
    cd $(lib_dir)/../../docker/compose-build && docker-compose up -d && cd -
}

docker_stop_build_container() {
    echo "Stop docker"
    cd $(lib_dir)/../../docker/compose-build && docker-compose stop && cd -
}

docker_clean_build_container() {
    echo "Clean docker"
    cd $(lib_dir)/../../docker/compose-build && docker-compose down && cd -
}
