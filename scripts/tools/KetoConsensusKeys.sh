#!/bin/bash

number=$1
directoryWithKeys=$2
outputFile=$3

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -f "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh" ] ; then
	exit 0
fi

privateKeyBytes=""
seperator=""
for (( count=1; count<=${number}; count++ ))
do
    echo "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -P -k \"${directoryWithKeys}/key_${count}.json\""
    privateBytes=`${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -P -k "${directoryWithKeys}/key_${count}.json"`
    echo "Bytes from file ${directoryWithKeys}/key_${count}.json : ${privateBytes}"
    privateKeyBytes="${privateKeyBytes}${seperator}${privateBytes}"
    seperator=","
done
echo "${privateKeyBytes}" > ${outputFile}
