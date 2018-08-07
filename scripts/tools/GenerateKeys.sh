#!/bin/bash

number=$1
outputDir=$2

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for (( count=1; count<=${number}; count++ ))
do
   echo "${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -G > \"${outputDir}/key_${count}.json\""
   ${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -G > "${outputDir}/key_${count}.json"
done
