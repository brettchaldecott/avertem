#!/bin/bash

VERSION=`cat ${KETO_HOME}/config/keto_version`
SERVER_VERSION=`wget -qO- https://s3-eu-west-1.amazonaws.com/keto-release/linux/ubuntu/latest_version.txt`
CURRENT_DIR=`pwd`

if [ "$VERSION" == "$SERVER_VERSION" ] ; then
    echo "Versions are the same nothing to be done"
    exit 0
fi

echo "Download the version $SERVER_VERSION"
wget https://s3-eu-west-1.amazonaws.com/keto-release/linux/ubuntu/$SERVER_VERSION/keto_shared_$SERVER_VERSION.tar.gz -O $KETO_HOME/tmp/keto_shared_$SERVER_VERSION.tar.gz

cd $KETO_HOME/shared/ && tar -zxvf $KETO_HOME/tmp/keto_shared_$SERVER_VERSION.tar.gz
cd $CURRENT_DIR
echo "$SERVER_VERSION" > ${KETO_HOME}/config/keto_version
echo "Upgrade complete"
