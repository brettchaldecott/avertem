#!/bin/bash

number=$1
directoryWithKeys=$2

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

privateKeyBytes=""
seperator=""
for (( count=1; count<=${number}; count++ ))
do
    privateBytes=`${SOURCE_DIR}/../../deps_build/build/install/bin/keto_tools.sh -P -k "${directoryWithKeys}/key_${count}.json"`
    privateKeyBytes="${privateKeyBytes}${seperator}${privateBytes}"
    seperator=","
done
echo "${privateKeyBytes}"
