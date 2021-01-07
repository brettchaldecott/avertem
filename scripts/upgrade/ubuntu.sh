#!/bin/bash

vercomp () {
    if [[ $1 == $2 ]]
    then
        return 0
    fi
    local IFS=.
    local i ver1=($1) ver2=($2)
    # fill empty fields in ver1 with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
    do
        ver1[i]=0
    done
    for ((i=0; i<${#ver1[@]}; i++))
    do
        if [[ -z ${ver2[i]} ]]
        then
            # fill empty fields in ver2 with zeros
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]}))
        then
            return 1
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]}))
        then
            return 2
        fi
    done
    return 0
}


VERSION=`cat ${KETO_HOME}/config/avertem_version`
if [ -z "$VERSION" ] ; then
    VERSION="1.0.0"
fi
SERVER_VERSION=`wget -qO- https://s3-eu-west-1.amazonaws.com/avertem/linux/ubuntu/18.04/latest_version.txt`
CURRENT_DIR=`pwd`

vercomp "${VERSION}" "${SERVER_VERSION}"
VERSION_RESULT=$?

if (( $VERSION_RESULT <= 1 )) ; then
    echo "Versions are the same nothing to be done"
    exit 0
fi


echo "Download the version $SERVER_VERSION"
wget https://s3-eu-west-1.amazonaws.com/avertem/linux/ubuntu/18.04/$SERVER_VERSION/avertem_shared_$SERVER_VERSION.tar.gz -O $KETO_HOME/tmp/avertem_shared_$SERVER_VERSION.tar.gz

cd $KETO_HOME/shared/ && tar -zxvf $KETO_HOME/tmp/avertem_shared_$SERVER_VERSION.tar.gz
cd $CURRENT_DIR
echo "$SERVER_VERSION" > ${KETO_HOME}/config/avertem_version
echo "Upgrade complete"
