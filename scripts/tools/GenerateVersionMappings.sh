#!/bin/bash

HEADER_FILES=(`find $1 -name "*.hpp" | xargs grep -l getHeaderVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 2-4 -d "/" -`)
SOURCE_CLASSES=(`find $1 -name "*.hpp" | xargs grep getSourceVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)
HEADER_CLASSES=(`find $1 -name "*.hpp" | xargs grep getHeaderVersion | sed "s/include/#/g" | cut -f 2 -d "#" - | cut -f 1 -d ":" - | cut -f 1 -d "." - | cut -f 2-4 -d "/" --output-delimiter='::' -`)
#HEADER_FILES=(`find $1 -name "*.hpp" | xargs grep getHeaderVersion | cut -f 1 -d ":" - | cut -f 2 -d "." - | cut -f 5-7 -d "/" --output-delimiter='::' -`)

#echo $HEADER_FILES

for headerEntry in "${HEADER_FILES[@]}";
do
   echo "#include \"$headerEntry\""
done
for classEntry in "${HEADER_CLASSES[@]}";
do
   echo "      sourceVersionMap[\"$classEntry::getHeaderVersion\"] = &$classEntry::getHeaderVersion;"
done
for classEntry in "${SOURCE_CLASSES[@]}";
do
   echo "      sourceVersionMap[\"$classEntry::getSourceVersion\"] = &$classEntry::getSourceVersion;"
done
