#!/bin/bash

number=$1
directoryWithKeys=$2

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

privateKeyBytes=""
for (( count=1; count<=${number}; count++ ))
do
    privateBytes=`${SOURCE_DIR}/../../deps_build/build/install/bin/keto_tools.sh -K -k "${directoryWithKeys}/key_${count}.json"`
    privateKeyBytes="${privateKeyBytes},${privateBytes}"
done
echo "${privateKeyBytes}"
