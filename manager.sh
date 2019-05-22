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
    echo "   start_cluster - start the cluster"
    echo "   stop_cluster - stop the cluster"
    echo "   clean_cluster - stop the cluster"
    exit -1
fi


if [ "${ACTION}" == "build" ] ;
then
    ./builds/scripts/docker_build.sh "build"
elif [ "${ACTION}" == "clean" ] ;
then
    ./builds/scripts/docker_build.sh "clean"
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
    ./builds/scripts/update_genisis.sh
elif [ "${ACTION}" == "start_cluster" ] ;
then
    cd docker/compose && docker-sync start && docker-compose up
elif [ "${ACTION}" == "stop_cluster" ] ;
then
    cd docker/compose && docker-compose stop && docker-sync stop
elif [ "${ACTION}" == "clean_cluster" ] ;
then
    cd docker/compose && docker-compose down && docker-sync clean
fi


