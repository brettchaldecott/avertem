#!/bin/bash

ACTION=$1
if [ -z "$ACTION" ] ;
then
    echo "Must select and ACTION"    
    echo "   build - build the source code using a docker container"
    echo "   clean - clean up the docker containers"
    echo "   native - build the source code directly no container"
    echo "   ide - configure the source code base for your ide"
    echo "   genesis - rebuild the contracts used by the genesis block"
    exit -1
fi


if [ "${ACTION}" == "build" ] ;
then
    ./builds/scripts/docker_build.sh "build"
elif [ "${ACTION}" == "clean" ] ;
then
    ./builds/scripts/docker_build.sh "clean"
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
    ./builds/scripts/docker_generator.sh "$ARCHITECTURE" "$IDE" "$COPY_DEPENDENCIES"

elif [ "${ACTION}" == "genesis" ] ;
then
    ./builds/scripts/update_genisis.sh
fi


