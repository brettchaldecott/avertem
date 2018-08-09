#!/bin/bash

sourceDir=$1
outputDir=$2

sourceKeys=(`find $sourceDir -name "*.json"`)
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


for sourceKey in "${sourceKeys[@]}";
do
    filename=${sourceKey##*/}
    echo "${SOURCE_DIR}/../../deps_build/build/install/bin/keto_tools.sh -G -k ${sourceKey} > \"${outputDir}/${filename}\""
    ${SOURCE_DIR}/../../deps_build/build/install/bin/keto_tools.sh -G -k ${sourceKey} > "${outputDir}/${filename}"
done
