#!/bin/bash

sourceDir=$1
outputDir=$2
overlap=$3

if [[ -z "${overlap}" ]]; then
    overlap=0
fi

sourceKeys=$(find $sourceDir -name "*.json" -print0 | sort -z)
number=$(find $sourceDir -name "*.json" | wc -l)
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

count=1;
for sourceKey in "${sourceKeys[@]}";
do
    if [[ "${count}" -ge "${overlap}" ]]; then
        filename=${sourceKey##*/}
        echo "${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G -k ${sourceKey} > \"${outputDir}/${filename}\""
        ${SOURCE_DIR}/../../deps_build/build/install/bin/avertem_tools.sh -G -k ${sourceKey} > "${outputDir}/${filename}"
    fi
    $(( count++ ))
done
