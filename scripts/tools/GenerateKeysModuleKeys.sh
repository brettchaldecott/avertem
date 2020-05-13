#!/bin/bash

sourceDir=$1
outputDir=$2
overlap=$3

if [[ -z "${overlap}" ]]; then
    overlap=0
fi

#sourceKeys=(`find $sourceDir -name "*.json" | sort -z`)
number=(`find $sourceDir -name "*.json" | wc -l`)
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -f "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh" ] ; then
        exit 0
fi

zero=0
if [[ "${overlap}" -ne "${zero}" ]]; then
    startPoint=$(( ${number} - ${overlap} ))
    for (( count=1; count<= ${overlap}; count++ ))
    do
        if [ ! -f "${outputDir}/key_$((${startPoint}+${count})).json" ] ; then
            echo "mv -f ${outputDir}/key_$((${startPoint}+${count})).json ${outputDir}/key_${count}.json"
            mv -f "${outputDir}/key_$((${startPoint}+${count})).json" "${outputDir}/key_${count}.json"
        fi
    done
fi

for (( count=${overlap}+1; count<= ${number}; count++ ))
do
    echo "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G -k \"${sourceDir}/key_${count}.json\" > \"${outputDir}/key_${count}.json\""
    ${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G -k "${sourceDir}/key_${count}.json" > "${outputDir}/key_${count}.json"
    echo "Processed entry ${count}"
done
