#!/bin/bash

VERSION=`cat ${KETO_HOME}/config/avertem_version`
SERVER_VERSION=`wget -qO- https://s3-eu-west-1.amazonaws.com/avertem-release/linux/ubuntu/latest_version.txt`
CURRENT_DIR=`pwd`

if [ "$VERSION" == "$SERVER_VERSION" ] ; then
    echo "Versions are the same nothing to be done"
    exit 0
fi

echo "Download the version $SERVER_VERSION"
wget https://s3-eu-west-1.amazonaws.com/avertem-release/linux/ubuntu/$SERVER_VERSION/avertem_shared_$SERVER_VERSION.tar.gz -O $KETO_HOME/tmp/avertem_shared_$SERVER_VERSION.tar.gz

cd $KETO_HOME/shared/ && tar -zxvf $KETO_HOME/tmp/avertem_shared_$SERVER_VERSION.tar.gz
cd $CURRENT_DIR
echo "$SERVER_VERSION" > ${KETO_HOME}/config/avertem_version
echo "Upgrade complete"
