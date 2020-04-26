#!/bin/bash

ACTION=$1
if [ -z "$ACTION" ] ;
then
    echo "Must select and ACTION"    
    echo "   image - build the required image"
    echo "   dev - build the source code using a docker container"
    echo "   clean - clean up the docker containers"
    echo "   native - build the source code directly no container"
    echo "   ide - configure the source code base for your ide"
    echo "   genesis - rebuild the contracts used by the genesis block"
    echo "   cluster - start the cluster"
    echo "   debian - build the debian cluster"
    exit -1
fi


if [ "${ACTION}" == "dev" ] ;
then
    ARGS=()
    for var in "$@"; do
        # Ignore known bad arguments
        [ "$var" != 'dev' ] && ARGS+=("$var")
    done
    ./builds/scripts/docker_build.sh "${ARGS[@]}"
elif [ "${ACTION}" == "clean" ] ;
then
    ./builds/scripts/docker_build.sh "clean"
    ./docker/scripts/dev_cluster.sh "clean"
    rm -rf ./build/*
    rm -rf ./ide_build/*
    rm -rf ./deps_build/build
elif [ "${ACTION}" == "native" ] ;
then
    ARCHITECTURE=$2
    BUILD_ACTION=$3
    if [ -z "$ARCHITECTURE" ] || [ -z "$BUILD_ACTION" ] ;
    then
        echo "native: must provide the [architecture] and [build action]"
        echo "   architecture - currently ubuntu"
        echo "   build_action - currently build"
        exit -1
    fi
    ./builds/scripts/build.sh "$ARCHITECTURE" "$BUILD_ACTION"
#elif [ "${ACTION}" == "ide" ] ;
#then
#    IDE=$2
#    COPY_DEPENDENCIES=$3
#    if [ -z "$IDE" ] ;
#    then
#        echo "native: must provide the [ide type]"
#        echo "   ide_type - currently build"
#        echo "   -d - copy dependencies"
#        exit -1
#    fi
#    ./builds/scripts/docker_generator.sh ide "$IDE" "$COPY_DEPENDENCIES"
#
elif [ "${ACTION}" == "ide" ] ;
then
    ARCHITECTURE=$2
    IDE=$3
    COPY_DEPENDENCIES=$4
    if [ -z "$ARCHITECTURE" ] || [ -z "$IDE" ] ;
    then
        echo "native: must provide the [architecture] and [ide type]"
        echo "   architecture - currently ubuntu"
        echo "   ide_type - currently build"
        exit -1
    fi
    ./builds/scripts/generator.sh "$ARCHITECTURE" "$IDE" "$COPY_DEPENDENCIES"

elif [ "${ACTION}" == "genesis" ] ;
then
    ./builds/scripts/docker_build.sh ${ACTION}
elif [ "${ACTION}" == "cluster" ] ;
then
    ARGS=()
    for var in "$@"; do
        # Ignore known bad arguments
        [ "$var" != 'cluster' ] && ARGS+=("$var")
    done
    ./docker/scripts/dev_cluster.sh "${ARGS[@]}"
elif [ "${ACTION}" == "debian" ] ;
then
    ARGS=()
    for var in "$@"; do
        # Ignore known bad arguments
        [ "$var" != 'debian' ] && ARGS+=("$var")
    done
    ./docker/scripts/debian_image.sh "${ARGS[@]}"
elif [ "${ACTION}" == "image" ] ;
then
    ARGS=()
    for var in "$@"; do
        # Ignore known bad arguments
        [ "$var" != 'image' ] && ARGS+=("$var")
    done
    ./docker/scripts/dev_image.sh "${ARGS[@]}"
elif [ "${ACTION}" == "node" ] ;
then
    ARGS=()
    for var in "$@"; do
        # Ignore known bad arguments
        [ "$var" != 'node' ] && ARGS+=("$var")
    done
    ./docker/scripts/node_cluster.sh "${ARGS[@]}"
fi


