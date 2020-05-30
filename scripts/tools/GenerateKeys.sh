#!/bin/bash

number=$1
outputDir=$2
overlap=$3

if [[ -z "${overlap}" ]]; then
    overlap=0
fi

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -f "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh" ] ; then
        exit 0
fi

zero=0
if [[ "${overlap}" -ne "${zero}" ]]; then
    startPoint=$(( ${number} - ${overlap} ))
    for (( count=1; count<= ${overlap}; count++ ))
    do
        echo "mv -f ${outputDir}/key_$((${startPoint}+${count})).json ${outputDir}/key_${count}.json"
        mv -f "${outputDir}/key_$((${startPoint}+${count})).json" "${outputDir}/key_${count}.json"
    done
fi



for (( count=${overlap}+1; count <= ${number}; count++ ))
do
   echo "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G > \"${outputDir}/key_${count}.json\""
   ${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G > "${outputDir}/key_${count}.json"
done
