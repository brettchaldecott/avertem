#!/bin/bash

number=$1
sourceFile=$2
outputDir=$3

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for (( count=1; count<=${number}; count++ ))
do
    echo "cp -f ${sourceFile} \"${outputDir}/consensus_${count}.chai\""
    cp -f ${sourceFile} "${outputDir}/consensus_${count}.chai"
done
