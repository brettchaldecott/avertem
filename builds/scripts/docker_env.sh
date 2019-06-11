#!/bin/bash

lib_dir() {
    echo "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
}

docker_check_build_container_env() {
    cd $(lib_dir)/../../docker/compose-build && echo "$(docker-compose ps -q)" && cd -
}

docker_start_build_container_env() {
    BUILD_STATUS="$(docker_check_build_container_env)"
    #if [ -z "${BUILD_STATUS}" ] ;
    #then
        echo "Start docker"
        cd $(lib_dir)/../../docker/compose-build && docker-sync start && docker-compose up -d && cd -
    #fi
}

docker_stop_build_container_env() {
    echo "Stop docker"
    cd $(lib_dir)/../../docker/compose-build && docker-compose stop && docker-sync stop && cd -
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
    docker_start_build_container_env
}

docker_stop_build_container() {
    echo "Build Environment remains running"
}

docker_clean_build_container() {
    echo "Clean docker"
    docker_stop_build_container_env
    cd $(lib_dir)/../../docker/compose-build && docker-compose down && docker-sync clean && cd -
}
